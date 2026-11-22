#include "processor/amd64_uart.h"
#include "processor/amd64_port.h"
#include "io/uart.h"

static
bool uart_probe(u16 base) {
    amd64_out_port8(base + UART_MODEM_CTRL, 0x1E);
    amd64_out_port8(base + UART_DATA_PORT, UART_TEST_BYTE_A);
    const bool ok = amd64_in_port8(base + UART_DATA_PORT) == UART_TEST_BYTE_A;
    amd64_out_port8(base + UART_MODEM_CTRL, 0x0F);
    return ok;
}

bool amd64_uart_init(u16 base, u16 divisor) {
    uart_probe(base);

    amd64_out_port8(base + UART_INTERRUPT_ENABLE, UART_DISABLE_INTERRUPTS);
    amd64_out_port8(base + UART_LINE_CTRL, UART_ENABLE_DLAB);
    amd64_out_port8(base + UART_DIVISOR_LOW, divisor & 0xFF);
    amd64_out_port8(base + UART_DIVISOR_HIGH, (divisor >> 8) & 0xFF);
    amd64_out_port8(base + UART_LINE_CTRL, UART_8N1);
    amd64_out_port8(base + UART_FIFO_CTRL, UART_FIFO_ENABLE_CLEAR_14);
    amd64_out_port8(base + UART_MODEM_CTRL, UART_MCR_IRQ_ENABLED);
}

void amd64_uart_write_byte(u16 base, u8 byte) {
    while (!(amd64_in_port8(base + UART_LINE_STATUS) & UART_TRANSMIT_READY))
        ;

    amd64_out_port8(base + UART_DATA_PORT, byte);
}