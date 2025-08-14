#include "time/clock.hpp"
#include "drivers/pit.hpp"

uint64_t clock_get_time_since_boot() {
    return pit_get_global_tick_count();
}