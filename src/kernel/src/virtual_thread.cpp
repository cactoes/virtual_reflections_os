#include "virtual_thread.hpp"
#include "std/map.hpp"
#include "utils/mutex.hpp"

#include "crash_handler.hpp"
#include "time/clock.hpp"
#include "std/string.hpp"
#include "cpu.hpp"
#include "linker.hpp"
#include "utils/debug.hpp"
#include "memory/heap.hpp"
#include "io.hpp"
#include "arch/amd64/idt.hpp"
#include "arch/amd64/vmem.hpp"

// TODO @since 23/10/2025 -- 19:06
// change into 1 "bigger" thread handler

static std::linear_map<vthread_handle_t, std::unique_ptr<vthread_t>> g_threads {};

static vthread_handle_t     g_vth_counter = 1;
static vthread_t*           g_current_thread = nullptr;

static volatile bool global_is_in_critical_section = false;
static volatile spinlock_t global_thread_lock {};

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

    spinlock_lock((spinlock_t*)&global_thread_lock);

    if (p_vthread->stack_bottom_kernel)
        free_aligned(p_vthread->stack_bottom_kernel);
    else
        free_aligned(p_vthread->stack_bottom);

    if (p_vthread->kstack)
        free_aligned(p_vthread->kstack);

    free_aligned(p_vthread->fpu_state);

    // TODO @since 09/05/2026 -- 20:56
    // properly free parent process
    if (p_vthread->parent)
        free(p_vthread->parent);

    g_threads.remove(p_vthread->handle);

    spinlock_unlock((spinlock_t*)&global_thread_lock);
}

void vthread_handle_starting(vthread_t* p_vthread) {
    // for now just promote to running
    // we dont yet need to do sht
    p_vthread->vt_state = vthread_state_t::RUNNING;
}

bool vthread_add(std::unique_ptr<vthread_t> p_vthread) {
    p_vthread->vt_state = vthread_state_t::RUNNING;

    spinlock_lock((spinlock_t*)&global_thread_lock);

    if (!g_threads.insert(p_vthread->handle, move(p_vthread))) {
        spinlock_unlock((spinlock_t*)&global_thread_lock);
        return false;
    }

    spinlock_unlock((spinlock_t*)&global_thread_lock);

    return true;
}

vthread_handle_t vthread_start_and_setup_main() {
    if (g_current_thread)
        return false;

    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();

    p_vthread->handle = VTHREAD_MAIN_THREAD_HANDLE;
    p_vthread->pml4 = amd64_get_page_table();
    p_vthread->tls.handle = VTHREAD_MAIN_THREAD_HANDLE;
    p_vthread->is_critical = true;

    const char name[] = "kernel_thread_main";
    memcpy(p_vthread->name, name, sizeof(name));
    
    p_vthread->fpu_state = (u8*)malloc_aligned(sizeof(u8) * 512, 16);

    g_current_thread = p_vthread.get();

    return vthread_add(move(p_vthread)) ? 0 : VTHREAD_HANDLE_INVALID;
}

vthread_handle_t vthread_create_local(thread_entry_t p_thread_entry, const char name[VTHREAD_MAX_NAME_SIZE]) {
    u64* stack = (u64*)malloc_aligned(VTHREAD_STACK_SIZE, 16);
    if (!stack)
        return VTHREAD_HANDLE_INVALID;

    memzero(stack, VTHREAD_STACK_SIZE);
    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();
    p_vthread->stack_bottom = stack;
    u64* stack_top = (u64*)(((u64)stack + VTHREAD_STACK_SIZE - sizeof(interrupt_regs_t)) & ~0xF);

    // itret frame
    *(--stack_top) = 0x10; // gdt_get_kernel_data_selector()
    *(--stack_top) = (u64)stack_top;
    *(--stack_top) = 0x202;
    *(--stack_top) = 0x8; // gdt_get_kernel_code_selector()
    *(--stack_top) = (u64)vthread_entry_point;
    *(--stack_top) = 0;

    // general registers
    for (int i = 0; i < 13; i++)
        *(--stack_top) = 0;

    // startup arguments for the loader
    *(--stack_top) = (u64)p_thread_entry;
    *(--stack_top) = 0;

    const vthread_handle_t new_handle = vhtread_next_handle();

    p_vthread->stack_top = stack_top;
    p_vthread->handle = new_handle;
    p_vthread->vt_state = vthread_state_t::RUNNING;
    p_vthread->fpu_state = (u8*)malloc_aligned(sizeof(u8) * 512, 16);
    p_vthread->pml4 = amd64_get_page_table();
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

void* vthread_handle_interrupt(void* stack, void*) {
    return vthread_schedule(stack);
}

bool vthread_check_stack(vthread_t* thread) {
    // stak has overflown
    if ((u64)thread->stack_top < (u64)thread->stack_bottom)
        return false;

    // thread stack is in deadzone, to prevent overflow we terminate it
    if ((u64)thread->stack_top - (u64)thread->stack_bottom < VTHREAD_STACK_DEADZONE)
        return false;

    // stack is still safe :)
    return true;
}

extern struct amd64_tss_t* amd64_get_tss();
extern void amd64_tss_set_stack_pointer0(struct amd64_tss_t* tss, void* stack_pointer);

void* vthread_schedule(void* stack) {
    if (g_threads.size() <= 1)
        return stack;

    g_current_thread->stack_top = (void*)stack;
    amd64_fpu_store(g_current_thread->fpu_state);

    do {
        g_current_thread = vthread_get_next_thead(g_current_thread->handle);

        if ((u64)g_current_thread < PAGE_SIZE_LARGE)
            debug_trap("invalid thread");

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

    set_current_process(g_current_thread->parent);

    amd64_set_page_table(g_current_thread->pml4);
    amd64_fpu_load(g_current_thread->fpu_state);

    // this means that the thread is a userprocess
    // & needs a kernel stack when an interrupt happens
    if (g_current_thread->kstack) {
        auto kstack_top = (void*)((u64)g_current_thread->kstack + VTHREAD_STACK_SIZE);
        amd64_tss_set_stack_pointer0(amd64_get_tss(), kstack_top);
        cpu_set_kernel_stack(get_current_cpu(), kstack_top);
    } else {
        // gdt_set_stack_pointer0(g_current_thread->stack_bottom);
        // set_kernel_stack(get_current_cpu(), g_current_thread->stack_bottom);
    }

    interrupt_regs_t* target_stack = (interrupt_regs_t*)g_current_thread->stack_top;

    // if (((target_stack->cs & ~3) >> 3) != USER_CODE_SELECTOR_INDEX && ((target_stack->cs & ~3) >> 3) != KERNEL_CODE_SELECTOR_INDEX)
    //     debug_trap("loading corrupted stack");

    return target_stack;
}

void vthread_yield() {
    // TODO @since 19/05/2026 -- 16:19
    // add a platform based selector
    amd64_call_scheduler_interrupt();
}

thread_local_storage_t* vthread_get_tls() {
    return g_current_thread ? &g_current_thread->tls : nullptr;
}

void vthread_sleep(u64 time_ms) {
    if (g_current_thread->handle == VTHREAD_MAIN_THREAD_HANDLE) {
        u64 sleep_until_ms = clock_get_time_since_boot() + time_ms;
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

// critical_section_t enter_critical_section(bool wait_for_lock, bool can_fail) {
//     cli();

//     critical_section_t critical_section {};
//     critical_section.is_locked = false;

//     if (global_is_in_critical_section) {
//         if (!wait_for_lock) {
//             if (can_fail) {
//                 sti();
//                 return critical_section;
//             }
    
//             sti();
//             kernel_fatal(KERNEL_FATAL_CRITICAL_SECTION_FAILED, "double critical section");
//         } else {
//             sti();
//             while (global_is_in_critical_section)
//                 vthread_yield();
//             cli();            
//         }
//     }

//     global_is_in_critical_section = true;
//     critical_section.is_locked = true;

//     sti();
//     return critical_section;
// }

// bool leave_critical_section(critical_section_t* section) {
//     cli();
//     if (!global_is_in_critical_section) {
//         sti();
//         return false;
//     }

//     section->is_locked = false;
//     global_is_in_critical_section = false;

//     sti();
//     return true;
// }

vthread_handle_t vhtread_next_handle() {
    return g_vth_counter++;
}