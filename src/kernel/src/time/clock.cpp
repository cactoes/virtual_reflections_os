#include "time/clock.hpp"
#include "drivers/pit.hpp"
#include "drivers/cmos.hpp"

uint64_t clock_get_time_since_boot() {
    return pit_get_global_tick_count();
}

uint64_t clock_get_current_time() {
    return cmos_read_time();
}