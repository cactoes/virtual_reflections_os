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

    vthread->cpu_state.rip = (uint64_t)entry;
    vthread->cpu_state.cs = 0x08;
    vthread->cpu_state.rflags = 0x202;

    memcpy((void*)((uint64_t)stack_top - sizeof(cpu_state_t)), &vthread->cpu_state, sizeof(cpu_state_t));
    
    return vthread_add(vthread);
}

cpu_state_t* vthread_schedule(cpu_state_t* cpu_state) {
    if (g_vthread_count == 0)
        return nullptr;

    auto current_vthread = g_vthreads[g_current_vthread_index];

    // store original cpu state
    memcpy(&current_vthread->cpu_state, cpu_state, sizeof(cpu_state_t));

    // get next thread
    g_current_vthread_index = (g_current_vthread_index + 1) % g_vthread_count;
    const auto next_vthread = g_vthreads[g_current_vthread_index];

    // TODO @since 14/04/2025 -- 14:02
    // validate if thread is asleep or not
    
    // return new cpu state
    // BUG @since 14/04/2025 -- 15:50
    // uses original stack and wont properly work
    memcpy(cpu_state, &next_vthread->cpu_state, sizeof(cpu_state_t));
    return cpu_state;

    // memcpy((void*)((uint64_t)next_vthread->stack + sizeof(cpu_state_t)), &next_vthread->cpu_state, sizeof(cpu_state_t));

    // // switch to next task
    // return (cpu_state_t*)next_vthread->stack;

    // const cpu_state_t* stack_old = (const cpu_state_t*)current_vthread->stack;
    // const cpu_state_t* stack_next = (const cpu_state_t*)next_vthread->stack;

    // memcpy(next_vthread->stack, &next_vthread->cpu_state, sizeof(cpu_state_t));

    // // Now return the new stack pointer (which should point to the saved state)
    // return (cpu_state_t*)next_vthread->stack;
}