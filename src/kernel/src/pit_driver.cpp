#include "pit_driver.hpp"
#include "cpu.hpp"
#include "virtual_thread.hpp"

static vector<pit_timer_t>* g_timers;

cpu_state_t* pit_handle_interrupt(uint64_t code, cpu_state_t* rsp) {
    for (VECTOR_LOOP(g_timers, timer_node))
        timer_node->value.tick++;

    if (const auto new_thread = vthread_schedule(rsp))
        return new_thread;

    return rsp;
}

uint32_t pit_read_count() {
    uint32_t count = 0;

    asm volatile ("cli");

    cpu_outb(0x43, 0x00);
    count = cpu_inb(0x40);
    count |= cpu_inb(0x40) << 8;

    asm volatile ("sti");

    return count;
}

void pit_set_count(uint32_t count) {
    asm volatile ("cli");

    cpu_outb(0x40, (uint8_t)(count & 0xFF));
    cpu_outb(0x40, (uint8_t)((count >> 8) & 0xFF));

    asm volatile ("sti");
}

void pit_sleep(uint32_t ms) {
    pit_sleep(((tls_base_t*)vthread_get_tls())->vtid, ms);
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
        while ( (start - timer->tick) < 1000 ) {}
    }
}

void pit_init(vector<pit_timer_t>* timers) {
    g_timers = timers;

    uint32_t divisor = 1193182 / 1000;
    cpu_outb(0x43, 0x36);
    pit_set_count(divisor);
}

void pit_add_clock(uint64_t id) {
    g_timers->insert_back(pit_timer_t { .id = id, .tick = 0 });
}