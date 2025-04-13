#include "virtual_thread.hpp"

bool vthread_add(vthread_t* vthread) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;
    
    g_vthreads[g_vthread_count++] = vthread;
    return true;
}

bool vthread_create(vthread_t* vthread, void(*entry)()) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    vthread->stack = heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);

    memzero(&vthread->cpu_state, sizeof(cpu_state_t));

    uint64_t* stack_top = (uint64_t*)((uint8_t*)vthread->stack + VTHREAD_STACK_SIZE);
    stack_top = (uint64_t*)((uint64_t)stack_top & ~0xF); // 16-byte align

    vthread->cpu_state.rsp = (uint64_t)stack_top;
    vthread->cpu_state.rip = (uint64_t)entry;
    vthread->cpu_state.res = 0x08;
    vthread->cpu_state.rflags = 0x202;
    
    return vthread_add(vthread);
}

cpu_state_t* vthread_schedule(cpu_state_t* cpu_state) {
    if (g_vthread_count == 0)
        return nullptr;

    // store original cpu state
    memcpy(&g_vthreads[g_current_vthread_index]->cpu_state, cpu_state, sizeof(cpu_state_t));
    g_vthreads[g_current_vthread_index]->stack = (void*)cpu_state;

    g_current_vthread_index = (g_current_vthread_index + 1) % g_vthread_count;

    auto vt = g_vthreads[g_current_vthread_index];
    
    // memcpy(vt->stack, &vt->cpu_state, sizeof(cpu_state_t));
    memcpy(cpu_state, &vt->cpu_state, sizeof(cpu_state_t));

    // switch to next task
    return cpu_state;
}