#include "vga_driver.hpp"
#include "string.hpp"

static vga_tm_color_map_t g_vga_tm_current_color {
    .color = (uint8_t)vga_tm_color_t::WHITE | ((uint8_t)vga_tm_color_t::BLACK << 4)
};

static uint32_t g_vga_tm_column = 0;
static uint32_t g_vga_tm_row = 0;

void vga_tm_new_line() {
    g_vga_tm_column = 0;

    if (g_vga_tm_row < VGA_TM_NUM_ROWS - 1) {
        g_vga_tm_row++;
        return;
    }
    
    volatile uint16_t* vga_tm_mem = (volatile uint16_t*)VGA_TM_BUFFER_ADDR;

    for (uint64_t r = 1; r < VGA_TM_NUM_ROWS; r++)
        for (uint64_t c = 0; c < VGA_TM_NUM_COLS; c++)
            vga_tm_mem[c + VGA_TM_NUM_COLS * (r - 1)] = vga_tm_mem[c + VGA_TM_NUM_COLS * r];

    (void)vga_tm_clear_row(VGA_TM_NUM_ROWS - 1);
}

int vga_tm_print(char ch) {
    if (g_vga_tm_column >= VGA_TM_NUM_COLS)
        vga_tm_new_line();

    volatile uint16_t* vga_tm_mem = (volatile uint16_t*)VGA_TM_BUFFER_ADDR;

    switch (ch) {
        case '\n':
            vga_tm_new_line();
            break;
        default:
            vga_tm_mem[g_vga_tm_column + VGA_TM_NUM_COLS * g_vga_tm_row] = ch | (g_vga_tm_current_color.color << 8);
            g_vga_tm_column++;
            break;
    }

    return 0;
}

int vga_tm_print(const char* fmt, ...) {
    if (fmt == nullptr || *fmt == 0)
        return 1;

    char buffer[512] = { 0 };

    va_list args;
    va_start(args, fmt);
    size_t strlen = sprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for (uint64_t i = 0; buffer[i] != '\0' && i < sizeof(buffer); i++)
        (void)vga_tm_print(buffer[i]);

    const int result = vga_tm_set_cursor(g_vga_tm_column, g_vga_tm_row);
    return result == 0 ? 0 : 2;
}

int vga_tm_print(const vga_tm_color_map_t* color_map, const char* fmt, ...) {
    if (fmt == nullptr || *fmt == 0 || color_map == nullptr)
        return 1;

    char buffer[512] = { 0 };

    va_list args;
    va_start(args, fmt);
    size_t strlen = sprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    vga_tm_color_map_t old_color;
    (void)vga_tm_get_color(&old_color);
    (void)vga_tm_set_color(color_map);

    for (uint64_t i = 0; buffer[i] != '\0' && i < sizeof(buffer); i++)
        (void)vga_tm_print(buffer[i]);

    (void)vga_tm_set_color(&old_color);

    const int result = vga_tm_set_cursor(g_vga_tm_column, g_vga_tm_row);
    return result == 0 ? 0 : 2;
}

int vga_tm_clear_row(uint32_t row) {
    if (row >= VGA_TM_NUM_ROWS)
        return 1;

    volatile uint16_t* vga_tm_mem = (volatile uint16_t*)VGA_TM_BUFFER_ADDR;

    for (uint32_t c = 0; c < VGA_TM_NUM_COLS; c++)
        vga_tm_mem[c + VGA_TM_NUM_COLS * row] = ' ' | (g_vga_tm_current_color.color << 8);

    const int result = vga_tm_set_cursor(0, row);
    return result == 0 ? 0 : 2;
}

int vga_tm_clear_screen() {
    for (uint32_t r = 0; r < VGA_TM_NUM_ROWS; r++)
        (void)vga_tm_clear_row(r);
    
    const int result = vga_tm_set_cursor(0, 0);
    return result == 0 ? 0 : 1;
}

int vga_tm_set_cursor(uint32_t x, uint32_t y) {
    // TODO @since 21/03/2025 -- 20:42
    return 1;
}

int vga_tm_get_cursor(uint32_t* x, uint32_t* y) {
    if (x == nullptr || y == nullptr)
        return 1;

    *x = g_vga_tm_column;
    *y = g_vga_tm_row;

    return 0;
}

int vga_tm_set_color(const vga_tm_color_map_t* color_map) {
    if (color_map == nullptr)
        return 1;

    g_vga_tm_current_color = *color_map;
    return 0;
}

int vga_tm_get_color(vga_tm_color_map_t* color_map) {
    if (color_map == nullptr)
        return 1;

    *color_map = g_vga_tm_current_color;
    return 0;
}