#include "utils/debug.hpp"

// TODO @since 21/05/2026 -- 22:29
// you know the drill
#include "arch/amd64/port.hpp"
#include "arch/amd64/cpu.hpp"

bool uart_is_transmit_ready(u16 base) {
    return (amd64_in_port8(base + UART_LINE_STATUS) & UART_TRANSMIT_READY) != 0;
}

void debug_putc(const char ch) {
    uart_putc(AMD64_COM1, ch);
}

void debug_puts(const char* str) {
    uart_puts(AMD64_COM1, str);
}

void debug_trap(const char* msg) {
    uart_puts(AMD64_COM1, "[debug trap hit: ");
    uart_puts(AMD64_COM1, msg);
    uart_puts(AMD64_COM1, "]");
#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_halt();
#endif
}

void uart_init(u16 base, u16 divisor) {
    // TODO @since 19/08/2026 -- 00:08
    // move actual implementation to amd64 folder

#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_out_port8(base + UART_INTERRUPT_ENABLE, UART_DISABLE_INTERRUPTS);
    amd64_out_port8(base + UART_LINE_CTRL, UART_ENABLE_DLAB);
    amd64_out_port8(base + UART_DATA_PORT, divisor & 0xFF);
    amd64_out_port8(base + UART_INTERRUPT_ENABLE, (divisor >> 8) & 0xFF);
    amd64_out_port8(base + UART_LINE_CTRL, UART_8N1);
    amd64_out_port8(base + UART_FIFO_CTRL, UART_FIFO_ENABLE_CLEAR_14);
    amd64_out_port8(base + UART_MODEM_CTRL, UART_MODEM_IRQ_RTS_DSR);
#endif
}

void uart_putc(u16 base, char ch) {
    while (!uart_is_transmit_ready(base))
        ;

    amd64_out_port8(base + UART_DATA_PORT, ch);
}

void uart_puts(u16 base, const char* str) {
    while (*str)
        debug_putc(*str++);
}