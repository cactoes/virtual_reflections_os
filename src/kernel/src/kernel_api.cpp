#include "kernel_api.hpp"
#include "drivers/vga.hpp"

int kernel_test_function(const char* p_str) {
    vga_tm_puts(&g_vga_tm_buffer, p_str);
    return 0;
}