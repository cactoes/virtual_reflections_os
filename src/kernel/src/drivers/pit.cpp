#include "drivers/pit.hpp"
#include "std/array.hpp"

#include "arch/amd64/pit.hpp"

static u64 g_tick_count = 0;
static std::dynamic_array<pit_interrupt_function_t> g_interrupt_functions {};

interrupt_regs_t* pit_handle_interrupt(interrupt_regs_t* p_cpu_state, void* data) {
    g_tick_count++;

    for (auto& func : g_interrupt_functions)
        p_cpu_state = func(p_cpu_state, data);

    return p_cpu_state;
}

u64 pit_get_global_tick_count() {
    return g_tick_count;
}

void pit_add_interrupt_function(pit_interrupt_function_t p_function) {
    g_interrupt_functions.insert_back(p_function);
}

void pit_init(u16 times_per_second) {
    amd64_pit_init(times_per_second);
}