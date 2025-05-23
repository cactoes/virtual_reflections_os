#include "pit_driver.hpp"
#include "cpu.hpp"
#include "virtual_thread.hpp"
#include "hardware_compatibility.hpp"

static vector<pit_timer_t>* g_timers;

cpu_state_t* pit_handle_interrupt(uint64_t code, cpu_state_t* rsp) {
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
    volatile pit_timer_t* timer = nullptr;

    for (VECTOR_LOOP(g_timers, timer_node)) {
        if (timer_node->value.id == id) {
            timer = &timer_node->value;
            break;
        }
    };

    if (!timer)
        return;

    for (uint32_t i = 0; i < ms; i++) {
        timer->tick = 0;
        uint64_t start = timer->tick;
        while ((start - timer->tick) < CLOCK_1MS) {}
    }
}

void pit_init(vector<pit_timer_t>* timers) {
    g_timers = timers;

    // 1000 = 1ms
    hc::pit::init(CLOCK_1MS);
}

void pit_add_clock(uint64_t id) {
    g_timers->insert_back(pit_timer_t { .id = id, .tick = 0 });
}