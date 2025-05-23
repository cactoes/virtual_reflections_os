#include "virtual_thread.hpp"
#include "hardware_compatibility.hpp"
#include "pit_driver.hpp"

static uint64_t g_vtid_counter = 1;
static vthread_t* g_current_thread = nullptr;

bool vthread_add(vthread_t* vthread) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;
    
    g_vthreads[g_vthread_count++] = vthread;
    pit_add_clock(vthread->vtid);
    return true;
}

bool vthread_create(vthread_t* vthread, void(*entry)()) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    uint64_t* stack = (uint64_t*)heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);

    if (!stack)
        return false;

    memzero(stack, VTHREAD_STACK_SIZE);

    uint64_t* sp = (uint64_t*)(((uint64_t)stack + VTHREAD_STACK_SIZE - sizeof(cpu_state_t)) & ~0xF);

    *(--sp) = (uint64_t)sp;
    *(--sp) = 0x202;
    *(--sp) = 0x8;
    *(--sp) = (uint64_t)entry;

    for (int i = 0; i < 14; i++)
        *(--sp) = 0;

    vthread->stack = sp;
    vthread->vtid = g_vtid_counter++;
    // for now RUNNING is fine since we dont use it
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

cpu_state_t* vthread_schedule(cpu_state_t* stack) {
    if (g_vthread_count == 0)
        return nullptr;

    g_vthreads[g_current_vthread_index]->stack = (void*)stack;

    vthread_loop_next_thread();

    // tss_set_rsp0(g_current_thread->stack);
    hc::gdt_tss::set_stack_pointer0(g_current_thread->stack);

    return (cpu_state_t*)g_current_thread->stack;
}

uint64_t* vthread_get_tls() {
    return g_current_thread->tls;
}