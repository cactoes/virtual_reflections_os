#include "virtual_thread.hpp"
#include "hardware_compatibility.hpp"
#include "pit_driver.hpp"

static uint64_t g_vtid_counter = 1;
static vthread_t* g_current_thread = nullptr;

bool vthread_add(vthread_t* vthread) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    vthread->vt_state = vthread_state_t::RUNNING;
    pit_add_clock(vthread->vtid);
    g_vthreads[g_vthread_count++] = vthread;
    return true;
}

void vthread_entry_point(void(*entry)()) {
    g_current_thread->vt_state = vthread_state_t::STARTING;
    entry();
    g_current_thread->vt_state = vthread_state_t::STOPPING;
    
    // catch it and wait for deletion
    while (true) {}
}

bool vthread_create(vthread_t* vthread, void(*entry)()) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    uint64_t* stack = (uint64_t*)heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);
    vthread->stack_og = stack;

    if (!stack)
        return false;

    memzero(stack, VTHREAD_STACK_SIZE);

    uint64_t* sp = (uint64_t*)(((uint64_t)stack + VTHREAD_STACK_SIZE - sizeof(cpu_state_t)) & ~0xF);

    *(--sp) = (uint64_t)sp;
    *(--sp) = 0x202;
    *(--sp) = 0x8;
    // *(--sp) = (uint64_t)entry;
    *(--sp) = (uint64_t)vthread_entry_point;

    for (int i = 0; i < 12; i++)
        *(--sp) = 0;

    *(--sp) =(uint64_t)entry;
    *(--sp) = 0;

    vthread->stack = sp;
    vthread->vtid = g_vtid_counter++;
    vthread->vt_state = vthread_state_t::RUNNING;
    ((tls_base_t*)vthread->tls)->vtid = vthread->vtid;

    if (vthread_add(vthread))
        return true;

    heap_free(get_global_heap(), stack);

    return false;
}

void vthread_loop_next_thread() {
    g_current_vthread_index = (g_current_vthread_index + 1) % g_vthread_count;
    g_current_thread = g_vthreads[g_current_vthread_index];
}

void vthread_handle_sleeping(vthread_t* thread) {
    const auto ptr = pit_find_by_id(thread->vtid);
    // if ptr is actually a nullptr there are bigger problems :p
    if (ptr->target_tick <= ptr->tick) {
        thread->vt_state = vthread_state_t::RUNNING;
        ((tls_base_t*)thread->tls)->is_yielded = 0;
    }
}

void vthread_handle_stopping(vthread_t* thread) {
    heap_free(get_global_heap(), thread->stack_og);

    // temp until we use a proper list
    g_vthreads[g_current_vthread_index] = nullptr;
}

void vthread_handle_starting(vthread_t* thread) {
    // for now just promote to running
    thread->vt_state = vthread_state_t::RUNNING;
}

cpu_state_t* vthread_schedule(cpu_state_t* stack) {
    if (g_vthread_count == 0)
        return nullptr;

    g_vthreads[g_current_vthread_index]->stack = (void*)stack;
    
    do {
        vthread_loop_next_thread();
        switch (g_current_thread->vt_state) {
            case vthread_state_t::STARTING: vthread_handle_starting(g_current_thread); break;
            case vthread_state_t::SLEEPING: vthread_handle_sleeping(g_current_thread); break;
            case vthread_state_t::STOPPING: vthread_handle_stopping(g_current_thread); break;
            case vthread_state_t::UNKNOWN:
            default: break;
        }
    } while (g_current_thread->vt_state != vthread_state_t::RUNNING);

    hc::gdt_tss::set_stack_pointer0(g_current_thread->stack);

    return (cpu_state_t*)g_current_thread->stack;
}

tls_base_t* vthread_get_tls() {
    return (tls_base_t*)g_current_thread->tls;
}

void vthread_yield() {
    vthread_get_tls()->is_yielded = 1;
    while (vthread_get_tls()->is_yielded)
        g_current_thread->vt_state = vthread_state_t::SLEEPING;
}