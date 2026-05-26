#include "drivers/pit.hpp"
#include "std/array.hpp"
#include "arch/amd64/pit.hpp"
#include "virtual_thread.hpp"

static u64 g_tick_count = 0;

void* pit_handle_interrupt(void* stack, void* data) {
    // update tick count
    g_tick_count++;

    // call scheduler & return (new) stack
    return vthread_handle_interrupt(stack, nullptr);
}

u64 pit_get_global_tick_count() {
    return g_tick_count;
}

void pit_init(u16 times_per_second) {
    amd64_pit_init(times_per_second);
}