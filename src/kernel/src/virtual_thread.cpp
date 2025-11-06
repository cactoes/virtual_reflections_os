#include "virtual_thread.hpp"
#include "std/map.hpp"
#include "utils/mutex.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"
#include "arch/generic.hpp"
#include "crash_handler.hpp"
#include "std/pointer.hpp"
#include "time/clock.hpp"

// TODO @since 23/10/2025 -- 19:06
// change into 1 "bigger" thread handler

static std::linear_map<vthread_handle_t, std::unique_ptr<vthread_t>> g_threads {};

static vthread_handle_t     g_vth_counter = 1;
static vthread_t*           g_current_thread = nullptr;

static mutex_t g_mutex {};

void vthread_entry_point(thread_entry_t p_thread_entry) {
    g_current_thread->vt_state = vthread_state_t::STARTING;
    while (g_current_thread->vt_state == vthread_state_t::STARTING)
        vthread_yield();
    
    g_current_thread->exit_code = p_thread_entry();
    g_current_thread->vt_state = vthread_state_t::STOPPING;

    // catch & wait for deletion
    while (true);
}

vthread_t* vthread_get_next_thead(vthread_handle_t handle) {
    mutex_lock_guard guard(&g_mutex);

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
    mutex_lock_guard guard(&g_mutex);

    heap_free(get_global_heap(), p_vthread->stack_bottom);
    g_threads.remove(p_vthread->handle);
}

void vthread_handle_starting(vthread_t* p_vthread) {
    // for now just promote to running
    // we dont yet need to do sht
    p_vthread->vt_state = vthread_state_t::RUNNING;
}

bool vthread_add(std::unique_ptr<vthread_t> p_vthread) {
    mutex_lock_guard guard(&g_mutex);

    p_vthread->vt_state = vthread_state_t::RUNNING;
    
    if (!g_threads.insert(p_vthread->handle, move(p_vthread)))
        return false;

    return true;
}

vthread_handle_t vthread_start_and_setup_main(file_descriptor_t out_streams[3]) {
    if (g_current_thread)
        return false;

    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();

    p_vthread->handle = VTHREAD_MAIN_THREAD_HANDLE;
    p_vthread->pml4 = get_pml4();
    p_vthread->tls.handle = VTHREAD_MAIN_THREAD_HANDLE;
    p_vthread->tls.out_streams[0] = out_streams[0];
    p_vthread->tls.out_streams[1] = out_streams[1];
    p_vthread->tls.out_streams[2] = out_streams[2];
    g_current_thread = p_vthread.get();

    return vthread_add(move(p_vthread)) ? 0 : VTHREAD_HANDLE_INVALID;
}

vthread_handle_t vthread_create(thread_entry_t p_thread_entry, void* pml4, file_descriptor_t out_streams[3]) {
    uint64_t* stack = (uint64_t*)heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);
    if (!stack)
        return VTHREAD_HANDLE_INVALID;

    memzero(stack, VTHREAD_STACK_SIZE);
    std::unique_ptr<vthread_t> p_vthread = std::make_unique<vthread_t>();
    p_vthread->stack_bottom = stack;
    uint64_t* stack_top = (uint64_t*)(((uint64_t)stack + VTHREAD_STACK_SIZE - sizeof(cpu_state_t)) & ~0xF);

    // itret frame
    *(--stack_top) = (uint64_t)stack_top;
    *(--stack_top) = 0x202;
    *(--stack_top) = 0x8;
    *(--stack_top) = (uint64_t)vthread_entry_point;
    *(--stack_top) = 0;

    // general registers
    for (int i = 0; i < 12; i++)
        *(--stack_top) = 0;

    // startup arguments for the loader
    *(--stack_top) = (uint64_t)p_thread_entry;
    *(--stack_top) = 0;

    const vthread_handle_t new_handle = g_vth_counter++;

    p_vthread->stack_top = stack_top;
    p_vthread->handle = new_handle;
    p_vthread->vt_state = vthread_state_t::RUNNING;
    p_vthread->pml4 = pml4;
    p_vthread->tls.handle = new_handle;
    p_vthread->tls.out_streams[0] = out_streams[0];
    p_vthread->tls.out_streams[1] = out_streams[1];
    p_vthread->tls.out_streams[2] = out_streams[2];

    if (vthread_add(move(p_vthread)))
        return new_handle;

    heap_free(get_global_heap(), stack);

    return VTHREAD_HANDLE_INVALID;
}

cpu_state_t* vthread_handle_interrupt(cpu_state_t* p_cpu_state) {
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

cpu_state_t* vthread_schedule(cpu_state_t* p_cpu_state) {  
    if (g_threads.size() == 0)
        return nullptr;

    g_current_thread->stack_top = (void*)p_cpu_state;

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

    gdt_set_stack_pointer0(g_current_thread->stack_top);
    set_pml4(g_current_thread->pml4);

    return (cpu_state_t*)g_current_thread->stack_top;
}

void vthread_yield() {
    call_scheduler_interrupt();
}

thread_local_storage_t* vthread_get_tls() {
    return &g_current_thread->tls;
}

void vthread_sleep(uint64_t time_ms) {
    // FIXME @since 06/11/2025 -- 17:05
    // can fully block the system
    // main thread has "busy" to sleep
    if (g_current_thread->handle == 0) {
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