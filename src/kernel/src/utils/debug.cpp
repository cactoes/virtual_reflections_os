#include "utils/debug.hpp"

// TODO @since 21/05/2026 -- 22:29
// you know the drill
#include "arch/amd64/port.hpp"
#include "arch/amd64/cpu.hpp"

int serial_is_transmit_ready() {
    return amd64_in_port8(DEBUG_PORT + DEBUG_SERIAL_LINE_STATUS) & DEBUG_SERIAL_TRANSMIT_READY;
}

void debug_init() {
    amd64_out_port8(DEBUG_PORT + DEBUG_SERIAL_INTERRUPT_ENABLE, DEBUG_SERIAL_DISABLE_INTERRUPTS);
    amd64_out_port8(DEBUG_PORT + DEBUG_SERIAL_LINE_CTRL, DEBUG_SERIAL_ENABLE_DLAB);
    amd64_out_port8(DEBUG_PORT + DEBUG_SERIAL_DATA_PORT, DEBUG_SERIAL_BAUD_DIVISOR_LOW);
    amd64_out_port8(DEBUG_PORT + DEBUG_SERIAL_INTERRUPT_ENABLE, DEBUG_SERIAL_BAUD_DIVISOR_HIGH);
    amd64_out_port8(DEBUG_PORT + DEBUG_SERIAL_LINE_CTRL, DEBUG_SERIAL_8N1);
    amd64_out_port8(DEBUG_PORT + DEBUG_SERIAL_FIFO_CTRL, DEBUG_SERIAL_FIFO_ENABLE_CLEAR_14);
    amd64_out_port8(DEBUG_PORT + DEBUG_SERIAL_MODEM_CTRL, DEBUG_SERIAL_MODEM_IRQ_RTS_DSR);
}

void debug_putc(const char ch) {
    while (!serial_is_transmit_ready()) {}
        amd64_out_port8(DEBUG_PORT, ch);
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
    amd64_halt();
}