#include "debug.hpp"
#include "cpu.hpp"
#include "string.hpp"

int serial_is_transmit_ready() {
    return cpu_inb(SERIAL_PORT + SERIAL_LINE_STATUS) & SERIAL_TRANSMIT_READY;
}

void debug_init() {
    cpu_outb(SERIAL_PORT + SERIAL_INTERRUPT_ENABLE, SERIAL_DISABLE_INTERRUPTS);
    cpu_outb(SERIAL_PORT + SERIAL_LINE_CTRL, SERIAL_ENABLE_DLAB);
    cpu_outb(SERIAL_PORT + SERIAL_DATA_PORT, SERIAL_BAUD_DIVISOR_LOW);
    cpu_outb(SERIAL_PORT + SERIAL_INTERRUPT_ENABLE, SERIAL_BAUD_DIVISOR_HIGH);
    cpu_outb(SERIAL_PORT + SERIAL_LINE_CTRL, SERIAL_8N1);
    cpu_outb(SERIAL_PORT + SERIAL_FIFO_CTRL, SERIAL_FIFO_ENABLE_CLEAR_14);
    cpu_outb(SERIAL_PORT + SERIAL_MODEM_CTRL, SERIAL_MODEM_IRQ_RTS_DSR);
}

void debug_print(const char ch) {
    while (!serial_is_transmit_ready()) {}
    cpu_outb(SERIAL_PORT, ch);
}

void debug_print(const char* fmt, ...) {
    char buffer[512] = { 0 };

    va_list args;
    va_start(args, fmt);
    size_t strlen = sprintf(buffer, (unsigned long int)sizeof(buffer), fmt, args);
    va_end(args);

    for (uint64_t i = 0; buffer[i] != '\0' && i < sizeof(buffer); i++)
        (void)debug_print(buffer[i]);
}