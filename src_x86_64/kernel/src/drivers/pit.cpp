#include "drivers/pit.hpp"
#include "std/array.hpp"

static uint64_t g_tick_count = 0;
static std::dynamic_array<pit_interrupt_function_t> g_interrupt_functions {};

cpu_state_t* pit_handle_interrupt(cpu_state_t* p_cpu_state) {
    g_tick_count++;

    for (auto& func : g_interrupt_functions)
        p_cpu_state = func(p_cpu_state);

    return p_cpu_state;
}

uint64_t pit_get_global_tick_count() {
    return g_tick_count;
}

void pit_add_interrupt_function(pit_interrupt_function_t p_function) {
    g_interrupt_functions.insert_back(p_function);
}