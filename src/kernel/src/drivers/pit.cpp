#include "drivers/pit.hpp"
#include "std/array.hpp"
#include "virtual_thread.hpp"

#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64
#include "arch/amd64/pit.hpp"
#endif

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
#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_pit_init(times_per_second);
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
}