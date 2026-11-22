#include "io/uart.h"
#include "processor/cpu_direct.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_uart.h"
#else
#error "undefined cpu architecture";
#endif

bool uart_init(u16 base, u16 divisor) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_uart_init(base, divisor);
#else
#endif
}

void uart_write_byte(u16 base, u8 byte) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_uart_write_byte(base, byte);
#else
#endif
}

void uart_write_string(u16 base, const char* str) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    for (; *str; str++) {
        if (*str == '\n')
            amd64_uart_write_byte(base, '\r');

        amd64_uart_write_byte(base, *str);
    }
#else
#endif
}