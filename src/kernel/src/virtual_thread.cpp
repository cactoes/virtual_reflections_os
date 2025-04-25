#include "virtual_thread.hpp"
#include "gdt.hpp"
#include "pit_driver.hpp"

static uint64_t g_vtid_counter = 1;
static vthread_t* g_current_thread = nullptr;

bool vthread_add(vthread_t* vthread) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;
    
    g_vthreads[g_vthread_count++] = vthread;
    return true;
}

bool vthread_create(vthread_t* vthread, void(*entry)()) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    uint64_t* stack = (uint64_t*)heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);

    if (!stack)
        return false;

    memzero(stack, VTHREAD_STACK_SIZE);

    uint64_t* sp = (uint64_t*)(((uint64_t)stack + sizeof(cpu_state_t)) & ~0xF);

    *(--sp) = (uint64_t)sp;
    *(--sp) = 0x202;
    *(--sp) = 0x8;
    *(--sp) = (uint64_t)entry;

    for (int i = 0; i < 14; i++)
        *(--sp) = 0;

    vthread->stack = sp;
    vthread->vtid = g_vtid_counter++;
    ((tls_base_t*)vthread->tls)->vtid = vthread->vtid;

    pit_add_clock(vthread->vtid);

    if (vthread_add(vthread))
        return true;

    heap_free(get_global_heap(), stack);

    return false;
}

cpu_state_t* vthread_schedule(cpu_state_t* stack) {
    if (g_vthread_count == 0)
        return nullptr;

    g_vthreads[g_current_vthread_index]->stack = (void*)stack;

    // get next thread
    g_current_vthread_index = (g_current_vthread_index + 1) % g_vthread_count;

    // TODO @since 14/04/2025 -- 14:02
    // validate if thread is asleep or not

    g_current_thread = g_vthreads[g_current_vthread_index];

    tss_set_rsp0(g_current_thread->stack);

    return (cpu_state_t*)g_current_thread->stack;
}

uint64_t* vthread_get_tls() {
    return g_current_thread->tls;
}