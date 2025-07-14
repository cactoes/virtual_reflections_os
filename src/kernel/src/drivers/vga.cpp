#include "drivers/vga.hpp"
#include "arch/generic.hpp"

vga_tm_buffer_t g_vga_tm_buffer {};

void vga_tm_init_buffer(vga_tm_buffer_t* p_buffer, void* p_vga_array, size_t width, size_t height) {
    p_buffer->buffer = (uint8_t*)p_vga_array;
    p_buffer->size.width = width;
    p_buffer->size.height = height;
    p_buffer->cursor.x = 0;
    p_buffer->cursor.y = 0;
    p_buffer->color.color = (uint8_t)vga_tm_color_t::WHITE | ((uint8_t)vga_tm_color_t::BLACK << 4);
}

void vga_tm_new_line(vga_tm_buffer_t* p_buffer) {
    p_buffer->cursor.x = 0;

    if (p_buffer->cursor.y < p_buffer->size.height - 1) {
        p_buffer->cursor.y++;
        return;
    }
    
    volatile uint16_t* vga_tm_mem = (volatile uint16_t*)p_buffer->buffer;

    for (uint64_t r = 1; r < p_buffer->size.height; r++)
        for (uint64_t c = 0; c < p_buffer->size.width; c++)
            vga_tm_mem[c + p_buffer->size.width * (r - 1)] = vga_tm_mem[c + p_buffer->size.width * r];

    (void)vga_tm_clear_row(p_buffer, p_buffer->size.height - 1);
}

int vga_tm_putc(vga_tm_buffer_t* p_buffer, char ch) {
    if (p_buffer->cursor.x >= p_buffer->size.width)
        vga_tm_new_line(p_buffer);

    volatile uint16_t* vga_tm_mem = (volatile uint16_t*)p_buffer->buffer;
    
    switch (ch) {
        case '\n':
            vga_tm_new_line(p_buffer);
            break;
        case '\r':
            p_buffer->cursor.x = 0;
            break;
        case '\b':
            if (p_buffer->cursor.x == 0 && p_buffer->cursor.y >= 1) {
                p_buffer->cursor.y--;
                p_buffer->cursor.x = p_buffer->size.width;
            }

            if (p_buffer->cursor.x >= 1)
                p_buffer->cursor.x--;
            vga_tm_mem[p_buffer->cursor.x + p_buffer->size.width * p_buffer->cursor.y] = ' ' | (p_buffer->color.color << 8);
            break;
        case '\t':
            for (int i = 0; i < 4; i++)
                vga_tm_putc(p_buffer, ' ');
            break;
        default:
            vga_tm_mem[p_buffer->cursor.x + p_buffer->size.width * p_buffer->cursor.y] = ch | (p_buffer->color.color << 8);
            p_buffer->cursor.x++;
            break;
    }

    return 0;
}

int vga_tm_puts(vga_tm_buffer_t* p_buffer, const char* p_str) {
    const char* p_ptr = p_str;
    while (*p_ptr)
        vga_tm_putc(p_buffer, *p_ptr++);

    const int result = vga_tm_set_cursor(p_buffer, p_buffer->cursor.x, p_buffer->cursor.y);
    return result == 0 ? 0 : 2;
}

int vga_tm_puts_color(vga_tm_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map, const char* p_str) {
    vga_tm_color_map_t old_color;
    (void)vga_tm_get_color(p_buffer, &old_color);
    (void)vga_tm_set_color(p_buffer, p_color_map);

    const char* p_ptr = p_str;
    while (*p_ptr)
        vga_tm_putc(p_buffer, *p_ptr++);

    (void)vga_tm_set_color(p_buffer, &old_color);

    const int result = vga_tm_set_cursor(p_buffer, p_buffer->cursor.x, p_buffer->cursor.y);
    return result == 0 ? 0 : 2;
}

int vga_tm_clear_row(vga_tm_buffer_t* p_buffer, uint32_t row) {
    if (row >= p_buffer->size.height)
        return 1;

    volatile uint16_t* vga_tm_mem = (volatile uint16_t*)p_buffer->buffer;

    for (uint32_t c = 0; c < p_buffer->size.width; c++)
        vga_tm_mem[c + p_buffer->size.width * row] = ' ' | (p_buffer->color.color << 8);

    const int result = vga_tm_set_cursor(p_buffer, 0, row);
    return result == 0 ? 0 : 2;
}

int vga_tm_clear_buffer(vga_tm_buffer_t* p_buffer) {
    for (uint32_t r = 0; r < p_buffer->size.height; r++)
        (void)vga_tm_clear_row(p_buffer, r);
    
    return vga_tm_set_cursor(p_buffer, 0, 0);
}

int vga_tm_set_cursor(vga_tm_buffer_t* p_buffer, uint32_t x, uint32_t y) {
    if (x > p_buffer->size.width || y > p_buffer->size.height)
        return 1;

    int position = x + p_buffer->size.width * y;

    out_port<uint8_t>(VGA_CRTC_INDEX, 14);
    out_port<uint8_t>(VGA_CRTC_DATA, (position >> 8) & 0xFF);
    out_port<uint8_t>(VGA_CRTC_INDEX, 15);
    out_port<uint8_t>(VGA_CRTC_DATA, position & 0xFF);

    p_buffer->cursor.x = x;
    p_buffer->cursor.y = y;

    return 0;
}

int vga_tm_get_cursor(vga_tm_buffer_t* p_buffer, uint32_t* p_x, uint32_t* p_y) {
    if (p_x == nullptr || p_y == nullptr)
        return 1;

    *p_x = p_buffer->cursor.x;
    *p_y = p_buffer->cursor.y;

    return 0;
}

int vga_tm_set_color(vga_tm_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map) {
    if (p_color_map == nullptr)
        return 1;

    p_buffer->color = *p_color_map;
    return 0;
}

int vga_tm_get_color(vga_tm_buffer_t* p_buffer, vga_tm_color_map_t* p_color_map) {
    if (p_color_map == nullptr)
        return 1;

    *p_color_map = p_buffer->color;
    return 0;
}