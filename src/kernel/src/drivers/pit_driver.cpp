#include "drivers/pit_driver.hpp"
#include "cpu.hpp"
#include "virtual_thread.hpp"
#include "hardware_compatibility.hpp"

static uint64_t g_global_tick_count;
static vector<pit_timer_t>* g_timers;

pit_timer_t* pit_find_by_id(uint64_t id) {
    for (VECTOR_LOOP(g_timers, timer_node)) {
        if (timer_node->value.id == id)
            return &timer_node->value;
    };

    return nullptr;
}

cpu_state_t* pit_handle_interrupt(uint64_t code, cpu_state_t* rsp) {
    g_global_tick_count++;

    for (VECTOR_LOOP(g_timers, timer_node))
        timer_node->value.tick++;

    if (const auto new_thread = vthread_schedule(rsp))
        return new_thread;

    return rsp;
}

void pit_sleep(uint32_t ms) {
    pit_sleep(vthread_get_tls()->vtid, ms);
}

void pit_sleep(uint64_t id, uint32_t ms) {
    volatile pit_timer_t* timer = pit_find_by_id(id);

    if (!timer)
        return;

    timer->tick = 0;
    timer->target_tick = ms;

    vthread_yield();
}

void pit_init(vector<pit_timer_t>* timers) {
    g_timers = timers;
    g_global_tick_count = 0;
    hc::pit::init(CLOCK_1MS);
}

void pit_add_clock(uint64_t id) {
    g_timers->insert_back(pit_timer_t { .id = id, .tick = 0 });
}

uint64_t pit_timer_read() {
    return g_global_tick_count;
}