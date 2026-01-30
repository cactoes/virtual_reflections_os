#include <stdarg.h>

#include "io.hpp"
#include "virtual_thread.hpp"
#include "utils/bitmap.hpp"
#include "utils/debug.hpp"
#include "drivers/vga.hpp"
#include "std/string.hpp"

static uint64_t global_io_bitmap[1];

bool write_stream(io_stream_t stream, const char* str) {
    switch (stream) {
        case io_stream_t::STD_ERR:
        case io_stream_t::STD_OUT:
        case io_stream_t::STD_WRN:
            return vga_tm_puts(&g_vga_tm_buffer, str) == 0;
        case io_stream_t::STD_DBG:
            debug_puts(str);
            return true;
        default:
            break;
    }

    return false;
}

void printf(const char* str, ...) {
    char buffer[IO_PRINT_BUFFER_SIZE] = { 0 };

    va_list args;
    va_start(args, str);
    sprintf(buffer, (unsigned long int)sizeof(buffer), str, args);
    va_end(args);

    write_stream(io_stream_t::STD_OUT, buffer);
}

void kprintf(const char* str, ...) {
    char buffer[IO_PRINT_BUFFER_SIZE] = { 0 };

    va_list args;
    va_start(args, str);
    sprintf(buffer, (unsigned long int)sizeof(buffer), str, args);
    va_end(args);

    write_stream(io_stream_t::STD_DBG, buffer);
}