#include "pit_driver.hpp"
#include "cpu.hpp"

static pit_timer_t* g_timers;
static size_t g_timers_size;

cpu_state_t* pit_handle_interrupt(uint64_t code, cpu_state_t* rsp) {
    for (size_t i = 0; i < g_timers_size; i++)
        g_timers[i].tick++;
    
    int_pic_send_eoi(IRQ_PIT);
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

void pit_sleep(uint64_t id, uint32_t ms) {
    // for (uint32_t i = 0; i < ms; i++) {
	
	// 	pit_set_count(1193182/1000);
	// 	uint32_t start = pit_read_count();

	// 	while ((start - pit_read_count()) < 1000) {}
	// }

    volatile pit_timer_t* timer = nullptr;

    for (size_t i = 0; i < g_timers_size; i++) {
        pit_timer_t& _timer = g_timers[i];

        if (_timer.id == id) {
            timer = &_timer;
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

void pit_init(pit_timer_t timers[], size_t size) {
    g_timers_size = size;
    g_timers = timers;

    uint32_t divisor = 1193182 / 1000;
    cpu_outb(0x43, 0x36);
    pit_set_count(divisor);
}