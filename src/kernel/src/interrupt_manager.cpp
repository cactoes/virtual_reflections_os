#include "interrupt_manager.hpp"

static interrupt_hook_t global_interrupt_hook_array[(u64)interrupt_t::SIZE] {};
volatile bool global_is_in_interupt = false;

interrupt_regs_t* interrupt_manager_dispatch(interrupt_t interrupt, interrupt_regs_t* stack) {
    // kprintf("unhandled interrupt triggerd: 0x%uh\n", code);
    auto& hook = global_interrupt_hook_array[(u64)interrupt];
    if (!hook.callback)
        return stack;

    return hook.callback(stack, hook.data);
}

bool hook_interrupt(interrupt_t code, interrupt_callback_t callback, void* data) {
    if ((u64)code >= (u64)interrupt_t::SIZE)
        return false;

    if (global_interrupt_hook_array[(u64)code].callback)
        return false;

    global_interrupt_hook_array[(u64)code] = {
        .callback = callback,
        .data = data
    };

    return true;
}

bool is_in_interrupt() {
    return global_is_in_interupt;
}