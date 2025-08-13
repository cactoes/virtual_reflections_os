#include "virtual_thread.hpp"
#include "utils/map.hpp"
#include "utils/mutex.hpp"
#include "arch/gdt.hpp"

static linear_map<vthread_handle_t, vthread_t*> g_threads {};

static vthread_handle_t     g_vth_counter = 1;
static vthread_t*           g_current_thread = nullptr;

static mutex_t g_mutex {};

void vthread_entry_point(thread_entry_t p_thread_entry) {
    g_current_thread->vt_state = vthread_state_t::STARTING;
    g_current_thread->exit_code = p_thread_entry();
    g_current_thread->vt_state = vthread_state_t::STOPPING;
    
    // catch & wait for deletion
    while (true);
}

void vthread_set_next_thead() {
    auto current_thread_it = g_threads.get(g_current_thread->handle);
    if (current_thread_it.advance() == g_threads.end())
        current_thread_it = g_threads.begin();
    
    g_current_thread = current_thread_it->value;
}

void vthread_handle_sleeping(vthread_t* p_vthread) {
    // TODO @since 13/08/2025 -- 23:38

    // const auto ptr = pit_find_by_id(thread->vtid);
    // // if ptr is actually a nullptr there are bigger problems :p
    // if (ptr->target_tick <= ptr->tick) {
    //     thread->vt_state = vthread_state_t::RUNNING;
    //     ((tls_base_t*)thread->tls)->is_yielded = 0;
    // }
}

void vthread_handle_stopping(vthread_t* p_vthread) {
    heap_free(get_global_heap(), p_vthread->stack_og);
    g_threads.remove(p_vthread->handle);
}

void vthread_handle_starting(vthread_t* p_vthread) {
    // for now just promote to running
    p_vthread->vt_state = vthread_state_t::RUNNING;
}

bool vthread_add(vthread_t* p_vthread) {
    if (!g_threads.insert(p_vthread->handle, p_vthread))
        return false;

    p_vthread->vt_state = vthread_state_t::RUNNING;
    return true;
}

bool vthread_start_and_setup_main(vthread_t* p_vthread) {
    mutex_lock_guard guard(&g_mutex);

    if (g_current_thread)
        return false;

    p_vthread->handle = 0;
    ((tls_base_t*)p_vthread->tls)->handle = 0;
    g_current_thread = p_vthread;
    return vthread_add(p_vthread);
}

bool vthread_create(vthread_t* p_vthread, thread_entry_t p_thread_entry) {
    mutex_lock_guard guard(&g_mutex);

    uint64_t* stack = (uint64_t*)heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);
    if (!stack)
        return false;

    memzero(stack, VTHREAD_STACK_SIZE);
    p_vthread->stack_og = stack;
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

    p_vthread->stack = stack_top;
    p_vthread->handle = g_vth_counter++;
    p_vthread->vt_state = vthread_state_t::RUNNING;
    ((tls_base_t*)p_vthread->tls)->handle = p_vthread->handle;

    if (vthread_add(p_vthread))
        return true;

    memzero(p_vthread, sizeof(vthread_t));
    heap_free(get_global_heap(), stack);

    return false;
}

cpu_state_t* vthread_interrupt_handler(cpu_state_t* p_cpu_state) {
    // TODO @since 13/08/2025 -- 23:37
    return p_cpu_state;
}

cpu_state_t* vthread_schedule(cpu_state_t* p_cpu_state) {
    mutex_lock_guard guard(&g_mutex);
    if (g_threads.size() == 0)
        return nullptr;

    g_current_thread->stack = (void*)p_cpu_state;
    
    do {
        vthread_set_next_thead();
        switch (g_current_thread->vt_state) {
            case vthread_state_t::STARTING: vthread_handle_starting(g_current_thread); break;
            case vthread_state_t::SLEEPING: vthread_handle_sleeping(g_current_thread); break;
            case vthread_state_t::STOPPING: vthread_handle_stopping(g_current_thread); break;
            case vthread_state_t::UNKNOWN:
            default: break;
        }
    } while (g_current_thread->vt_state != vthread_state_t::RUNNING);

    gdt_set_stack_pointer0(g_current_thread->stack);

    return (cpu_state_t*)g_current_thread->stack;
}

void vthread_yield() {
    // TODO @since 13/08/2025 -- 23:50
    // swap to interrupt based

    vthread_get_tls()->is_yielded = 1;
    while (vthread_get_tls()->is_yielded)
        g_current_thread->vt_state = vthread_state_t::SLEEPING;
}

tls_base_t* vthread_get_tls() {
    return (tls_base_t*)g_current_thread->tls;
}