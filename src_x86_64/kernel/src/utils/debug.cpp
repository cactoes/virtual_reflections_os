#include "utils/debug.hpp"
#include "arch/generic.hpp"

int serial_is_transmit_ready() {
    return in_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_LINE_STATUS) & DEBUG_SERIAL_TRANSMIT_READY;
}

void debug_init() {
    out_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_INTERRUPT_ENABLE, DEBUG_SERIAL_DISABLE_INTERRUPTS);
    out_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_LINE_CTRL, DEBUG_SERIAL_ENABLE_DLAB);
    out_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_DATA_PORT, DEBUG_SERIAL_BAUD_DIVISOR_LOW);
    out_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_INTERRUPT_ENABLE, DEBUG_SERIAL_BAUD_DIVISOR_HIGH);
    out_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_LINE_CTRL, DEBUG_SERIAL_8N1);
    out_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_FIFO_CTRL, DEBUG_SERIAL_FIFO_ENABLE_CLEAR_14);
    out_port<uint8_t>(DEBUG_PORT + DEBUG_SERIAL_MODEM_CTRL, DEBUG_SERIAL_MODEM_IRQ_RTS_DSR);
}

void debug_putc(const char ch) {
    while (!serial_is_transmit_ready()) {}
        out_port<uint8_t>(DEBUG_PORT, ch);
}

void debug_puts(const char* p_str) {
    const char* p_ptr = p_str;
    while (*p_ptr)
        debug_putc(*p_ptr++);
}

void debug_trap(const char* msg) {
    debug_puts("[debug trap hit: ");
    debug_puts(msg);
    debug_puts("]");
    halt();
}