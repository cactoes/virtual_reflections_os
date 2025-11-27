#include "drivers/vga.hpp"
#include "arch/generic.hpp"
#include "memory/heap.hpp"

vga_buffer_t g_vga_tm_buffer {};
static vga_buffer_t* g_vga_gm_buffer = nullptr;


void vga_tm_init_buffer(vga_buffer_t* p_buffer, void* p_vga_array, size_t width, size_t height) {
    p_buffer->buffer = (uint8_t*)p_vga_array;
    p_buffer->size.width = width;
    p_buffer->size.height = height;
    p_buffer->cursor.x = 0;
    p_buffer->cursor.y = 0;
    p_buffer->color.color = (uint8_t)vga_tm_color_t::WHITE | ((uint8_t)vga_tm_color_t::BLACK << 4);
}

void vga_tm_new_line(vga_buffer_t* p_buffer) {
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

int vga_tm_putc(vga_buffer_t* p_buffer, char ch) {
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

int vga_tm_puts(vga_buffer_t* p_buffer, const char* p_str) {
    const char* p_ptr = p_str;
    while (*p_ptr)
        vga_tm_putc(p_buffer, *p_ptr++);

    const int result = vga_tm_set_cursor(p_buffer, p_buffer->cursor.x, p_buffer->cursor.y);
    return result == 0 ? 0 : 2;
}

int vga_tm_puts_color(vga_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map, const char* p_str) {
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

int vga_tm_clear_row(vga_buffer_t* p_buffer, uint32_t row) {
    if (row >= p_buffer->size.height)
        return 1;

    volatile uint16_t* vga_tm_mem = (volatile uint16_t*)p_buffer->buffer;

    for (uint32_t c = 0; c < p_buffer->size.width; c++)
        vga_tm_mem[c + p_buffer->size.width * row] = ' ' | (p_buffer->color.color << 8);

    const int result = vga_tm_set_cursor(p_buffer, 0, row);
    return result == 0 ? 0 : 2;
}

int vga_tm_clear_buffer(vga_buffer_t* p_buffer) {
    for (uint32_t r = 0; r < p_buffer->size.height; r++)
        (void)vga_tm_clear_row(p_buffer, r);

    return vga_tm_set_cursor(p_buffer, 0, 0);
}

int vga_tm_set_cursor(vga_buffer_t* p_buffer, uint32_t x, uint32_t y) {
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

int vga_tm_get_cursor(vga_buffer_t* p_buffer, uint32_t* p_x, uint32_t* p_y) {
    if (p_x == nullptr || p_y == nullptr)
        return 1;

    *p_x = p_buffer->cursor.x;
    *p_y = p_buffer->cursor.y;

    return 0;
}

int vga_tm_set_color(vga_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map) {
    if (p_color_map == nullptr)
        return 1;

    p_buffer->color = *p_color_map;
    return 0;
}

int vga_tm_get_color(vga_buffer_t* p_buffer, vga_tm_color_map_t* p_color_map) {
    if (p_color_map == nullptr)
        return 1;

    *p_color_map = p_buffer->color;
    return 0;
}

void vga_gm_set_palette_color(uint8_t color_index, uint8_t red, uint8_t green, uint8_t blue) {
    out_port<uint8_t>(0x03C8, color_index);
    out_port<uint8_t>(0x03C9, red >> 2);
    out_port<uint8_t>(0x03C9, green >> 2);
    out_port<uint8_t>(0x03C9, blue >> 2);
}

void vga_gm_startup(vga_buffer_t* p_back_buffer) {
    out_port<uint8_t>(VGA_MISC_PORT, 0x63);

    out_port<uint8_t>(VGA_SEQ_INDEX, 0x00); out_port<uint8_t>(VGA_SEQ_DATA, 0x03);
    out_port<uint8_t>(VGA_SEQ_INDEX, 0x01); out_port<uint8_t>(VGA_SEQ_DATA, 0x01);
    out_port<uint8_t>(VGA_SEQ_INDEX, 0x02); out_port<uint8_t>(VGA_SEQ_DATA, 0x0F);
    out_port<uint8_t>(VGA_SEQ_INDEX, 0x03); out_port<uint8_t>(VGA_SEQ_DATA, 0x00);
    out_port<uint8_t>(VGA_SEQ_INDEX, 0x04); out_port<uint8_t>(VGA_SEQ_DATA, 0x0E);

    out_port<uint8_t>(VGA_CRTC_INDEX, 0x11); out_port<uint8_t>(VGA_CRTC_DATA, 0x00);

    static const uint8_t s_crtc_320x200[25] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF
    };

    for (uint8_t i = 0; i < ARRAY_SIZE(s_crtc_320x200); i++) {
        out_port<uint8_t>(VGA_CRTC_INDEX, i);
        out_port<uint8_t>(VGA_CRTC_DATA, s_crtc_320x200[i]);
    }

    out_port<uint8_t>(VGA_GC_INDEX, 0x00); out_port<uint8_t>(VGA_GC_DATA, 0x00);
    out_port<uint8_t>(VGA_GC_INDEX, 0x01); out_port<uint8_t>(VGA_GC_DATA, 0x00);
    out_port<uint8_t>(VGA_GC_INDEX, 0x02); out_port<uint8_t>(VGA_GC_DATA, 0x00);
    out_port<uint8_t>(VGA_GC_INDEX, 0x03); out_port<uint8_t>(VGA_GC_DATA, 0x00);
    out_port<uint8_t>(VGA_GC_INDEX, 0x04); out_port<uint8_t>(VGA_GC_DATA, 0x00);
    out_port<uint8_t>(VGA_GC_INDEX, 0x05); out_port<uint8_t>(VGA_GC_DATA, 0x40);
    out_port<uint8_t>(VGA_GC_INDEX, 0x06); out_port<uint8_t>(VGA_GC_DATA, 0x05);
    out_port<uint8_t>(VGA_GC_INDEX, 0x07); out_port<uint8_t>(VGA_GC_DATA, 0x0F);
    out_port<uint8_t>(VGA_GC_INDEX, 0x08); out_port<uint8_t>(VGA_GC_DATA, 0xFF);

    for (uint8_t i = 0; i < 16; i++) {
        in_port<uint8_t>(VGA_INSTAT_READ);
        out_port<uint8_t>(VGA_AC_INDEX, i);
        out_port<uint8_t>(VGA_AC_WRITE, i);
    }

    in_port<uint8_t>(VGA_INSTAT_READ); out_port<uint8_t>(VGA_AC_INDEX, 0x10); out_port<uint8_t>(VGA_AC_WRITE, 0x41);
    in_port<uint8_t>(VGA_INSTAT_READ); out_port<uint8_t>(VGA_AC_INDEX, 0x11); out_port<uint8_t>(VGA_AC_WRITE, 0x00);
    in_port<uint8_t>(VGA_INSTAT_READ); out_port<uint8_t>(VGA_AC_INDEX, 0x12); out_port<uint8_t>(VGA_AC_WRITE, 0x0F);
    in_port<uint8_t>(VGA_INSTAT_READ); out_port<uint8_t>(VGA_AC_INDEX, 0x13); out_port<uint8_t>(VGA_AC_WRITE, 0x00);
    in_port<uint8_t>(VGA_INSTAT_READ); out_port<uint8_t>(VGA_AC_INDEX, 0x14); out_port<uint8_t>(VGA_AC_WRITE, 0x00);

    in_port<uint8_t>(VGA_INSTAT_READ);
    out_port<uint8_t>(VGA_AC_INDEX, 0x20);

    static const uint8_t s_ega16[16][3] = {
        {  0,  0,  0}, {  0,  0,170}, {  0,170,  0}, {  0,170,170},
        {170,  0,  0}, {170,  0,170}, {170, 85,  0}, {170,170,170},
        { 85, 85, 85}, { 85, 85,255}, { 85,255, 85}, { 85,255,255},
        {255, 85, 85}, {255, 85,255}, {255,255, 85}, {255,255,255}
    };

    for (uint8_t i = 0; i < 16; i++)
        vga_gm_set_palette_color(i, s_ega16[i][0], s_ega16[i][1], s_ega16[i][2]);

    memzero((void*)VGA_GM_BUFFER_ADDR, VGA_GM_BUFFER_WIDTH * VGA_GM_BUFFER_HEIGHT * sizeof(uint8_t));
    g_vga_gm_buffer = const_cast<vga_buffer_t*>(p_back_buffer);
}

bool vga_gm_buffer_create(vga_buffer_t* p_back_buffer) {
    if (!p_back_buffer)
        return false;

    constexpr size_t buffer_size = sizeof(uint8_t) * VGA_GM_BUFFER_HEIGHT * VGA_GM_BUFFER_WIDTH;
    p_back_buffer->buffer = (uint8_t*)malloc(buffer_size);

    if (!p_back_buffer->buffer)
        return false;

    p_back_buffer->size.width = VGA_GM_BUFFER_WIDTH;
    p_back_buffer->size.height = VGA_GM_BUFFER_HEIGHT;
    memzero(p_back_buffer->buffer, buffer_size);

    return true;
}

void vga_gm_buffer_destroy(vga_buffer_t* p_back_buffer) {
    if (p_back_buffer && p_back_buffer->buffer) {
        free(p_back_buffer->buffer);
        p_back_buffer->buffer = nullptr;
        p_back_buffer->size.width = 0;
        p_back_buffer->size.height = 0;
    }
}

bool vga_gm_render() {
    if (!IS_VALID_BUFFER(g_vga_gm_buffer))
        return false;

    memcpy((void*)VGA_GM_BUFFER_ADDR, (void*)g_vga_gm_buffer->buffer, g_vga_gm_buffer->size.width * g_vga_gm_buffer->size.height * sizeof(uint8_t));

    return true;
}

bool vga_gm_swap_back_buffer(vga_buffer_t** p_back_buffer_new, vga_buffer_t** p_back_buffer_old) {
    if (!p_back_buffer_new || !*p_back_buffer_new)
        return false;

    if (p_back_buffer_old)
        *p_back_buffer_old = g_vga_gm_buffer;

    g_vga_gm_buffer = *p_back_buffer_new;

    return true;
}

bool vga_gm_draw::pixel(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer))
        return false;
    
    if (x >= VGA_GM_BUFFER_WIDTH || y >= VGA_GM_BUFFER_HEIGHT)
        return false;

    p_back_buffer->buffer[y * VGA_GM_BUFFER_WIDTH + x] = (uint8_t)color_index;

    return true;
}

bool vga_gm_draw::lineh(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, size_t len, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer) || x >= VGA_GM_BUFFER_WIDTH || y >= VGA_GM_BUFFER_HEIGHT)
        return false;

    for (size_t i = x; i < x + len && i < VGA_GM_BUFFER_WIDTH; i++)
        p_back_buffer->buffer[y * VGA_GM_BUFFER_WIDTH + i] = (uint8_t)color_index;

    return true;
}

bool vga_gm_draw::linev(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, size_t len, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer) || x >= VGA_GM_BUFFER_WIDTH || y >= VGA_GM_BUFFER_HEIGHT)
        return false;

    for (size_t i = y; i < y + len && i < VGA_GM_BUFFER_HEIGHT; i++)
        p_back_buffer->buffer[i * VGA_GM_BUFFER_WIDTH + x] = (uint8_t)color_index;

    return true;
}

bool vga_gm_draw::square(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, size_t w, size_t h, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer) || x >= VGA_GM_BUFFER_WIDTH || y >= VGA_GM_BUFFER_HEIGHT)
        return false;

    size_t max_x = x + w;
    size_t max_y = y + h;

    if (max_x > VGA_GM_BUFFER_WIDTH)
        max_x = VGA_GM_BUFFER_WIDTH;
    if (max_y > VGA_GM_BUFFER_HEIGHT)
        max_y = VGA_GM_BUFFER_HEIGHT;

    for (size_t j = y; j < max_y; j++) {
        for (size_t i = x; i < max_x; i++) {
            p_back_buffer->buffer[j * VGA_GM_BUFFER_WIDTH + i] = (uint8_t)color_index;
        }
    }

    return true;
}

bool vga_gm_draw::clear(vga_buffer_t* p_back_buffer, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer))
        return false;
    
    memset(p_back_buffer->buffer, (uint8_t)color_index, sizeof(uint8_t) * p_back_buffer->size.width * p_back_buffer->size.height);
    return true;
}