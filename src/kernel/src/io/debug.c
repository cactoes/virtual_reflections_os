#include "libstring.h"
#include "io/debug.h"
#include "io/uart.h"
#include "processor/cpu_direct.h"
#include "sync/spinlock.h"

static spinlock_t kdbg_lock = { 0 };

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_uart.h"
#else
#error "undefined cpu architecture";
#endif

bool kdbg_init() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return uart_init(AMD64_COM1_BASE, UART_BAUD_115200);
#else
#endif
}

void kdbg_printf(const char* fmt, ...) {
    char buffer[256];

    va_list ap;
    va_start(ap, fmt);
    vsprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    spinlock_lock(&kdbg_lock);

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    uart_write_string(AMD64_COM1_BASE, buffer);
#else
#endif

    spinlock_unlock(&kdbg_lock);
}

void kdbg_uprintf(const char* fmt, ...) {
    char buffer[256];

    va_list ap;
    va_start(ap, fmt);
    vsprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    uart_write_string(AMD64_COM1_BASE, buffer);
#else
#endif
}