#include "virtual_thread.hpp"
#include "std/map.hpp"
#include "utils/mutex.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"
#include "arch/generic.hpp"
#include "crash_handler.hpp"
#include "time/clock.hpp"
#include "std/string.hpp"
#include "cpu.hpp"
#include "linker.hpp"
#include "utils/debug.hpp"

// TODO @since 23/10/2025 -- 19:06
// change into 1 "bigger" thread handler

static std::linear_map<vthread_handle_t, std::unique_ptr<vthread_t>> g_threads {};

static vthread_handle_t     g_vth_counter = 1;
static vthread_t*           g_current_thread = nullptr;

static volatile bool global_is_in_critical_section = false;

void vthread_entry_point(thread_entry_t p_thread_entry) {
    g_current_thread->vt_state = vthread_state_t::STARTING;
    while (g_current_thread->vt_state == vthread_state_t::STARTING)
        vthread_yield();
    
    g_current_thread->exit_code = p_thread_entry();
    g_current_thread->vt_state = vthread_state_t::STOPPING;

    // catch & wait for deletion
    while (true);

    // FIXME @since 23/04/2026 -- 13:22
    // yield?
}

vthread_t* vthread_get_next_thead(vthread_handle_t handle) {
    // mutex_lock_guard guard(&g_mutex);

    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end() || current_thread_it.advance() == g_threads.end())
        current_thread_it = g_threads.begin();
    
    return current_thread_it->value.get();
}

void vthread_handle_sleeping(vthread_t* p_vthread) {
    if (p_vthread->sleep_until_ms <= clock_get_time_since_boot())
        p_vthread->vt_state = vthread_state_t::RUNNING;
}

void vthread_handle_stopping(vthread_t* p_vthread) {
    if (p_vthread->is_critical) {
        char buffer[256];
        (void)sprintf(buffer, sizeof(buffer), "critical thread died! (%s)", p_vthread->name ? p_vthread->name : "unknown");
        kernel_fatal(KERNEL_FATAL_CRITICAL_THREAD_DIED, buffer);
    }

    if (p_vthread->stack_bottom_kernel)
        free_aligned(p_vthread->stack_bottom_kernel);
    else
        free(p_vthread->stack_bottom);

    if (p_vthread->kstack)
        free_aligned(p_vthread->kstack);

    free_aligned(p_vthread->fpu_state);

    critical_section_t section = enter_critical_section();
    g_threads.remove(p_vthread->handle);
    leave_critical_section(&section);
}

void vthread_handle_starting(vthread_t* p_vthread) {
    // for now just promote to running
    // we dont yet need to do sht
    p_vthread->vt_state = vthread_state_t::RUNNING;
}

bool vthread_add(std::unique_ptr<vthread_t> p_vthread) {
    // mutex_lock_guard guard(&g_mutex);

    p_vthread->vt_state = vthread_state_t::RUNNING;

    critical_section_t section = enter_critical_section();

    if (!g_threads.insert(p_vthread->handle, move(p_vthread))) {
        leave_critical_section(&section);
        return false;
    }

    leave_critical_section(&section);

    return true;
}

vthread_handle_t vthread_start_and_setup_main() {
    if (g_current_thread)
        return false;

    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();

    p_vthread->handle = VTHREAD_MAIN_THREAD_HANDLE;
    p_vthread->pml4 = get_pml4();
    p_vthread->tls.handle = VTHREAD_MAIN_THREAD_HANDLE;

    const char name[] = "main";
    memcpy(p_vthread->name, name, sizeof(name));
    
    p_vthread->fpu_state = (uint8_t*)malloc_aligned(sizeof(uint8_t) * 512, 16);

    g_current_thread = p_vthread.get();

    return vthread_add(move(p_vthread)) ? 0 : VTHREAD_HANDLE_INVALID;
}

vthread_handle_t vthread_create(thread_entry_t p_thread_entry, void* pml4, const char name[VTHREAD_MAX_NAME_SIZE]) {
    uint64_t* stack = (uint64_t*)malloc(VTHREAD_STACK_SIZE);
    if (!stack)
        return VTHREAD_HANDLE_INVALID;

    memzero(stack, VTHREAD_STACK_SIZE);
    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();
    p_vthread->stack_bottom = stack;
    uint64_t* stack_top = (uint64_t*)(((uint64_t)stack + VTHREAD_STACK_SIZE - sizeof(interrupt_regs_t)) & ~0xF);

    // padding since the next section is only 19 * 8
    // which means its no longer 16-byte alignd
    *(--stack_top);

    // itret frame
    *(--stack_top) = (uint64_t)stack_top;
    *(--stack_top) = 0x202;
    *(--stack_top) = gdt_get_kernel_code_selector();
    *(--stack_top) = (uint64_t)vthread_entry_point;
    *(--stack_top) = 0;

    // general registers
    for (int i = 0; i < 12; i++)
        *(--stack_top) = 0;

    // startup arguments for the loader
    *(--stack_top) = (uint64_t)p_thread_entry;
    *(--stack_top) = 0;

    const vthread_handle_t new_handle = vhtread_next_handle();

    p_vthread->stack_top = stack_top;
    p_vthread->handle = new_handle;
    p_vthread->vt_state = vthread_state_t::RUNNING;
    p_vthread->fpu_state = (uint8_t*)malloc_aligned(sizeof(uint8_t) * 512, 16);
    p_vthread->pml4 = pml4;
    p_vthread->tls.handle = new_handle;

    if (name) {
        const size_t name_length = strlen(name);
        memzero(p_vthread->name, VTHREAD_MAX_NAME_SIZE + 1);
        memcpy(p_vthread->name, name, MIN(name_length, VTHREAD_MAX_NAME_SIZE));
    }

    if (vthread_add(move(p_vthread)))
        return new_handle;

    free(stack);

    return VTHREAD_HANDLE_INVALID;
}

interrupt_regs_t* vthread_handle_interrupt(interrupt_regs_t* p_cpu_state) {
    return vthread_schedule(p_cpu_state);
}

bool vthread_check_stack(vthread_t* thread) {
    // stak has overflown
    if ((uint64_t)thread->stack_top < (uint64_t)thread->stack_bottom)
        return false;

    // thread stack is in deadzone, to prevent overflow we terminate it
    if ((uint64_t)thread->stack_top - (uint64_t)thread->stack_bottom < VTHREAD_STACK_DEADZONE)
        return false;

    // stack is still safe :)
    return true;
}

interrupt_regs_t* vthread_schedule(interrupt_regs_t* p_cpu_state) {
    if (g_threads.size() <= 1)
        return p_cpu_state;

    // we are in critical section so do NOT mess with it until its done
    if (global_is_in_critical_section)
        return p_cpu_state;

    g_current_thread->stack_top = (void*)p_cpu_state;
    fpu_store(g_current_thread->fpu_state);

    do {
        g_current_thread = vthread_get_next_thead(g_current_thread->handle);

        switch (g_current_thread->vt_state) {
            case vthread_state_t::STARTING: vthread_handle_starting(g_current_thread); break;
            case vthread_state_t::SLEEPING: vthread_handle_sleeping(g_current_thread); break;
            case vthread_state_t::STOPPING: {
                vthread_handle_stopping(g_current_thread);
                g_current_thread = vthread_get_next_thead(VTHREAD_HANDLE_INVALID);
                break;
            }
            case vthread_state_t::RUNNING: {
                if (!vthread_check_stack(g_current_thread) && g_current_thread->handle != VTHREAD_MAIN_THREAD_HANDLE)
                    kernel_fatal(KERNEL_FATAL_VTHREAD_STACK_PROTECTION, "thread stack protection triggerd");
                break;
            }
            case vthread_state_t::UNKNOWN:
            default: break;
        }
    } while (g_current_thread->vt_state != vthread_state_t::RUNNING);

    set_pml4(g_current_thread->pml4);
    fpu_load(g_current_thread->fpu_state);

    // this means that the thread is a userprocess
    // & needs a kernel stack when an interrupt happens
    if (g_current_thread->kstack) {
        gdt_set_stack_pointer0(g_current_thread->kstack);
        set_kernel_stack(get_current_cpu(), g_current_thread->kstack);
    }

    return (interrupt_regs_t*)g_current_thread->stack_top;
}

void vthread_yield() {
    call_scheduler_interrupt();
}

thread_local_storage_t* vthread_get_tls() {
    return g_current_thread ? &g_current_thread->tls : nullptr;
}

void vthread_sleep(uint64_t time_ms) {
    if (g_current_thread->handle == VTHREAD_MAIN_THREAD_HANDLE) {
        uint64_t sleep_until_ms = clock_get_time_since_boot() + time_ms;
        while (clock_get_time_since_boot() < sleep_until_ms)
            vthread_yield();

        return;
    }

    // the rest can just yield until ready
    g_current_thread->sleep_until_ms = clock_get_time_since_boot() + time_ms;
    g_current_thread->vt_state = vthread_state_t::SLEEPING;
    vthread_yield();
}

int vthread_wait_for_close(vthread_handle_t handle) {
    while (g_threads.contains(handle));
    return 0;
}

size_t vthread_get_count() {
    return g_threads.size();
}

void vthread_terminate(vthread_handle_t handle) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return;

    current_thread_it->value->vt_state = vthread_state_t::STOPPING;
}

void vthread_terminate() {
    g_current_thread->vt_state = vthread_state_t::STOPPING;
    vthread_yield();
}

bool vthread_is_closed(vthread_handle_t handle) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return true;

    return current_thread_it->value->vt_state == vthread_state_t::STOPPING;
}

vthread_t* vthread_get(vthread_handle_t handle) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return nullptr;

    return current_thread_it->value.get();
}

bool vthread_set_critical(vthread_handle_t handle, bool state) {
    auto current_thread_it = g_threads.get(handle);
    if (current_thread_it == g_threads.end())
        return false;

    current_thread_it->value->is_critical = state;

    return true;
}

vthread_state_t vthread_get_state() {
    if (!g_current_thread)
        return vthread_state_t::UNKNOWN;

    return g_current_thread->vt_state;
}

critical_section_t enter_critical_section(bool wait_for_lock, bool can_fail) {
    cli();

    critical_section_t critical_section {};
    critical_section.is_locked = false;

    if (global_is_in_critical_section) {
        if (!wait_for_lock) {
            if (can_fail) {
                sti();
                return critical_section;
            }
    
            sti();
            kernel_fatal(KERNEL_FATAL_CRITICAL_SECTION_FAILED, "double critical section");
        } else {
            sti();
            while (global_is_in_critical_section)
                vthread_yield();
            cli();            
        }
    }

    global_is_in_critical_section = true;
    critical_section.is_locked = true;

    sti();
    return critical_section;
}

bool leave_critical_section(critical_section_t* section) {
    cli();
    if (!global_is_in_critical_section) {
        sti();
        return false;
    }

    section->is_locked = false;
    global_is_in_critical_section = false;

    sti();
    return true;
}

vthread_handle_t vhtread_next_handle() {
    return g_vth_counter++;
}