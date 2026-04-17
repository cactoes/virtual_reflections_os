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
        case io_stream_t::STD_WRN: {
            if (!strff(str, '\033'))
                return vga_tm_puts(&g_vga_tm_buffer, str) == 0;

            bool is_ansi_sequence = false;
            bool is_ansi_color_sequence = false;
            bool is_ansi_color_sequence_fg = true;
            char ansi_num_buf[4] = {0};
            int  ansi_num_len = 0;
            vga_tm_color_map_t color_map;
            vga_tm_get_color(&g_vga_tm_buffer, &color_map);

            size_t len = strlen(str);
            for (size_t i = 0; i < len; i++) {
                char ch = str[i];

                if (ch == '\033') {
                    is_ansi_sequence = true;
                    continue;
                }
                if (ch == '[' && is_ansi_sequence) {
                    is_ansi_color_sequence = true;
                    ansi_num_len = 0;
                    continue;
                }
                if (is_ansi_color_sequence && char_is_num(ch)) {
                    if (ansi_num_len < 3)
                        ansi_num_buf[ansi_num_len++] = ch;
                    continue;
                }

                if (is_ansi_color_sequence && (ch == ';' || ch == 'm')) {
                    ansi_num_buf[ansi_num_len] = '\0';
                    int code = atoi(ansi_num_buf);
                    ansi_num_len = 0;

                    if (code == 0) {
                        color_map.foreground = vga_tm_color_t::WHITE;
                        color_map.background = vga_tm_color_t::BLACK;
                    } else if (code >= 30 && code <= 37) {
                        color_map.foreground = (vga_tm_color_t)(code - 30);
                    } else if (code >= 90 && code <= 97) {
                        color_map.foreground = (vga_tm_color_t)(code - 90 + 8);
                    } else if (code >= 40 && code <= 47) {
                        color_map.background = (vga_tm_color_t)(code - 40);
                    } else if (code >= 100 && code <= 107) {
                        color_map.background = (vga_tm_color_t)(code - 100 + 8);
                    }

                    if (ch == 'm') {
                        vga_tm_set_color(&g_vga_tm_buffer, &color_map);
                        is_ansi_sequence = false;
                        is_ansi_color_sequence = false;
                        is_ansi_color_sequence_fg = true;
                    }

                    continue;
                }

                vga_tm_putc(&g_vga_tm_buffer, ch);
            }
            return true;
        }
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