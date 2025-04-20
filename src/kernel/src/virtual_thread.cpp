#include "virtual_thread.hpp"

bool vthread_add(vthread_t* vthread) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;
    
    g_vthreads[g_vthread_count++] = vthread;
    return true;
}

// bool vthread_create(vthread_t* vthread, void(*entry)()) {
//     if (g_vthread_count >= VTHREAD_MAX_COUNT)
//         return false;

//     vthread->stack = heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE);

//     memzero(&vthread->cpu_state, sizeof(cpu_state_t));

//     vthread->stack = (uint64_t*)((uint64_t)vthread->stack + VTHREAD_STACK_SIZE);
//     // stack_top = (uint64_t*)((uint64_t)stack_top & ~0xF);

//     ((cpu_state_t*)vthread->stack)->rip = (uint64_t)entry;
//     ((cpu_state_t*)vthread->stack)->rip = 0x08;
//     ((cpu_state_t*)vthread->stack)->rip = 0x202;

//     // vthread->cpu_state.rip = (uint64_t)entry;
//     // vthread->cpu_state.cs = 0x08;
//     // vthread->cpu_state.rflags = 0x202;

//     // memcpy((void*)((uint64_t)stack_top - sizeof(cpu_state_t)), &vthread->cpu_state, sizeof(cpu_state_t));

//     // vthread->stack = (void*)((uint64_t)stack_top - sizeof(cpu_state_t));

//     return vthread_add(vthread);
// }

bool vthread_create(vthread_t* vthread, void(*entry)()) {
    if (g_vthread_count >= VTHREAD_MAX_COUNT)
        return false;

    uint64_t* stack = (uint64_t*)heap_alloc(get_global_heap(), VTHREAD_STACK_SIZE * sizeof(uint64_t));

    if (!stack)
        return false;

    memzero(stack, VTHREAD_STACK_SIZE);

    uint64_t* sp = (uint64_t*)((uint64_t*)stack + VTHREAD_STACK_SIZE);
    sp = (uint64_t*)((uint64_t)sp & ~0xF);

    *(--sp) = 0x202;
    *(--sp) = 0x8;
    *(--sp) = (uint64_t)entry;

    for (int i = 0; i < 14; i++)
        *(--sp) = 0;

    vthread->stack = sp;

    if (vthread_add(vthread))
        return true;

    heap_free(get_global_heap(), stack);

    return false;
}

cpu_state_t* vthread_schedule(cpu_state_t* cpu_state) {
    if (g_vthread_count == 0)
        return nullptr;

    g_vthreads[g_current_vthread_index]->stack = cpu_state;

    // get next thread
    g_current_vthread_index = (g_current_vthread_index + 1) % g_vthread_count;

    // TODO @since 14/04/2025 -- 14:02
    // validate if thread is asleep or not

    return (cpu_state_t*)g_vthreads[g_current_vthread_index]->stack;
}