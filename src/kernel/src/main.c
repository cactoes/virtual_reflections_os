typedef unsigned uint32_t;

#define UART0_BASE  0x09000000UL
#define UART_DR     (*(volatile uint32_t *)(UART0_BASE + 0x000))
#define UART_FR     (*(volatile uint32_t *)(UART0_BASE + 0x018))
#define UART_IBRD   (*(volatile uint32_t *)(UART0_BASE + 0x024))
#define UART_LCRH   (*(volatile uint32_t *)(UART0_BASE + 0x02C))
#define UART_CR     (*(volatile uint32_t *)(UART0_BASE + 0x030))
#define UART_FR_TXFF (1 << 5)

static void uart_init(void) {
    UART_CR   = 0;          // disable UART
    UART_IBRD = 1;          // baud rate divisor
    UART_LCRH = (3 << 5);  // 8-bit words
    UART_CR   = (1 << 0) | (1 << 8) | (1 << 9);  // enable UART, TX, RX
}

static void uart_putc(char c) {
    while (UART_FR & UART_FR_TXFF);
    UART_DR = (uint32_t)c;
}

static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void kernel_main(void) {
    uart_init();
    uart_puts("VirtualReflectionsOS booting...\n");
    uart_puts("Hello from AArch64!\n");
    while (1) {}
}