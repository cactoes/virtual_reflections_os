#include "common.hpp"
#include "uart.hpp"

extern "C" void kernel_entry(void) {
    uart_init();

    uart_puts("VirtualReflectionsOS\n");
    uart_puts("Copyright (C) Blackline Technologies Ltd. System booted succesfully.\n");
    while (true);
}