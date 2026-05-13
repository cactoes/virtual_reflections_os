#include <stdarg.h>

#include "io.hpp"
#include "virtual_thread.hpp"
#include "utils/bitmap.hpp"
#include "utils/debug.hpp"
#include "drivers/vga.hpp"
#include "std/string.hpp"
#include "drivers/graphics/graphics_driver.hpp"

static uint64_t global_io_bitmap[1];

#define TERM_FONT_SIZE 8

struct test_terminal_t {
    color_t color;

    struct {
        size_t width;
        size_t height;
    } size;
    
    struct {
        size_t x;
        size_t y;
    } cursor;
} global_test_terminal;

void test_terminal_newline(test_terminal_t* term) {
    graphics_driver_t* gd = get_global_graphics_driver();

    graphics_driver_draw_square(gd, term->cursor.x, term->cursor.y, TERM_FONT_SIZE, TERM_FONT_SIZE, { 0, 0, 0 });

    term->cursor.x = 0;

    if (term->cursor.y + TERM_FONT_SIZE < term->size.height - TERM_FONT_SIZE) {
        term->cursor.y += TERM_FONT_SIZE;
        return;
    }

    graphics_driver_move_square(gd, 0, TERM_FONT_SIZE, term->size.width, (term->size.height / TERM_FONT_SIZE - 1) * TERM_FONT_SIZE, 0, 0);
    graphics_driver_draw_square(gd, 0, (term->size.height / TERM_FONT_SIZE - 1) * TERM_FONT_SIZE, term->size.width, TERM_FONT_SIZE, { 0, 0, 0 });
}

bool test_terminal_putc(test_terminal_t* term, char ch) {
    graphics_driver_t* gd = get_global_graphics_driver();
    if (!term || !gd)
        return false;

    if (term->cursor.x + TERM_FONT_SIZE >= term->size.width)
        test_terminal_newline(term);

    switch (ch) {
        case '\n':
            test_terminal_newline(term);
            break;
        case '\r':
            term->cursor.x = 0;
            break;
        case '\b':
            if (term->cursor.x >= TERM_FONT_SIZE)
                term->cursor.x -= TERM_FONT_SIZE;
            graphics_driver_draw_character(gd, term->cursor.x, term->cursor.y, ' ', term->color, 1.f);
            break;
        case '\t':
            for (size_t i = 0; i < 4; i++)
                test_terminal_putc(term, ' ');
            break;
        default:
            graphics_driver_draw_character(gd, term->cursor.x, term->cursor.y, ch, term->color, 1.f);
            term->cursor.x += TERM_FONT_SIZE;
            break;
    }

    graphics_driver_draw_character(gd, term->cursor.x, term->cursor.y, '_', { 255, 255, 255 }, 1.f);
    return true;
}

bool term_puts(test_terminal_t* term, const char* str) {
    while (*str)
        test_terminal_putc(term, *str++);
    return true;
}

bool io_term_init(size_t w, size_t h) {
    memzero(&global_test_terminal, sizeof(test_terminal_t));
    global_test_terminal.size.width = w;
    global_test_terminal.size.height = h;
    global_test_terminal.color = { 255, 255, 255 };
    return true;
}

constexpr color_t vga_to_rgb(vga_tm_color_t color) {
    switch (color) {
        case vga_tm_color_t::BLACK:       return {0x00, 0x00, 0x00};
        case vga_tm_color_t::BLUE:        return {0x00, 0x00, 0xAA};
        case vga_tm_color_t::GREEN:       return {0x00, 0xAA, 0x00};
        case vga_tm_color_t::CYAN:        return {0x00, 0xAA, 0xAA};
        case vga_tm_color_t::RED:         return {0xAA, 0x00, 0x00};
        case vga_tm_color_t::MAGENTA:     return {0xAA, 0x00, 0xAA};
        case vga_tm_color_t::BROWN:       return {0xAA, 0x55, 0x00};
        case vga_tm_color_t::LIGHT_GRAY:  return {0xAA, 0xAA, 0xAA};
        case vga_tm_color_t::DARK_GRAY:   return {0x55, 0x55, 0x55};
        case vga_tm_color_t::LIGHT_BLUE:  return {0x55, 0x55, 0xFF};
        case vga_tm_color_t::LIGHT_GREEN: return {0x55, 0xFF, 0x55};
        case vga_tm_color_t::LIGHT_CYAN:  return {0x55, 0xFF, 0xFF};
        case vga_tm_color_t::LIGHT_RED:   return {0xFF, 0x55, 0x55};
        case vga_tm_color_t::PINK:        return {0xFF, 0x55, 0xFF};
        case vga_tm_color_t::YELLOW:      return {0xFF, 0xFF, 0x55};
        case vga_tm_color_t::WHITE:       return {0xFF, 0xFF, 0xFF};
    }

    return {0x00, 0x00, 0x00};
}

bool write_stream(io_stream_t stream, const char* str) {
    switch (stream) {
        case io_stream_t::STD_ERR:
        case io_stream_t::STD_OUT:
        case io_stream_t::STD_WRN: {
            if (!strff(str, '\033'))
                return term_puts(&global_test_terminal, str) == 0;

            bool is_ansi_sequence = false;
            bool is_ansi_color_sequence = false;
            bool is_ansi_color_sequence_fg = true;
            char ansi_num_buf[4] = {0};
            int  ansi_num_len = 0;
            color_t color_map = global_test_terminal.color;

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
                        color_map = { 255, 255, 255 };
                    } else if (code >= 30 && code <= 37) {
                        color_map = vga_to_rgb((vga_tm_color_t)(code - 30));
                    } else if (code >= 90 && code <= 97) {
                        color_map = vga_to_rgb((vga_tm_color_t)(code - 90 + 8));
                    } else if (code >= 40 && code <= 47) {
                        color_map = vga_to_rgb((vga_tm_color_t)(code - 40));
                    } else if (code >= 100 && code <= 107) {
                        color_map = vga_to_rgb((vga_tm_color_t)(code - 100 + 8));
                    }

                    if (ch == 'm') {
                        global_test_terminal.color = color_map;
                        is_ansi_sequence = false;
                        is_ansi_color_sequence = false;
                        is_ansi_color_sequence_fg = true;
                    }

                    continue;
                }

                test_terminal_putc(&global_test_terminal, ch);
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