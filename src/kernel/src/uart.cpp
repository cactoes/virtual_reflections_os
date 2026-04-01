#include "uart.hpp"

void uart_write(uint32_t addr, uint32_t value) {
    (*(volatile uint32_t*)(UART_BASE + addr)) = value;
}

uint32_t uart_read(uint32_t addr) {
    return (*(volatile uint32_t*)(UART_BASE + addr));
}

bool uart_init() {
    uart_write(UART_CR, 0);
    uart_write(UART_IBRD, 1);
    uart_write(UART_LCRH, UART_LCRH_WLEN8 | UART_LCRH_FEN);
    uart_write(UART_CR, UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
    return true;
}

void uart_putc(char c) {
    while (uart_read(UART_FR) & UART_FR_TXFF);

    uart_write(UART_DR, (uint32_t)c);
}

void uart_puts(const char* str) {
    while (*str) {
        if (*str == '\n')
            uart_putc('\r');

        uart_putc(*str++);
    }
}