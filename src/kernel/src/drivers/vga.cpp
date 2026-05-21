#include "drivers/vga.hpp"

#include "memory/heap.hpp"
// TODO @since 21/05/2026 -- 22:29
// you know the drill
#include "arch/amd64/port.hpp"

vga_buffer_t g_vga_tm_buffer {};
static vga_buffer_t* g_vga_gm_buffer = nullptr;


void vga_tm_init_buffer(vga_buffer_t* p_buffer, void* p_vga_array, size_t width, size_t height) {
    p_buffer->buffer = (u8*)p_vga_array;
    p_buffer->size.width = width;
    p_buffer->size.height = height;
    p_buffer->cursor.x = 0;
    p_buffer->cursor.y = 0;
    p_buffer->color.color = (u8)vga_tm_color_t::WHITE | ((u8)vga_tm_color_t::BLACK << 4);
}

void vga_tm_new_line(vga_buffer_t* p_buffer) {
    p_buffer->cursor.x = 0;

    if (p_buffer->cursor.y < p_buffer->size.height - 1) {
        p_buffer->cursor.y++;
        return;
    }
    
    volatile u16* vga_tm_mem = (volatile u16*)p_buffer->buffer;

    for (u64 r = 1; r < p_buffer->size.height; r++)
        for (u64 c = 0; c < p_buffer->size.width; c++)
            vga_tm_mem[c + p_buffer->size.width * (r - 1)] = vga_tm_mem[c + p_buffer->size.width * r];

    (void)vga_tm_clear_row(p_buffer, p_buffer->size.height - 1);
}

int vga_tm_putc(vga_buffer_t* p_buffer, char ch) {
    if (p_buffer->cursor.x >= p_buffer->size.width)
        vga_tm_new_line(p_buffer);

    volatile u16* vga_tm_mem = (volatile u16*)p_buffer->buffer;
    
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

    return vga_tm_set_cursor(p_buffer, p_buffer->cursor.x, p_buffer->cursor.y);
}

int vga_tm_puts(vga_buffer_t* p_buffer, const char* p_str) {
    const char* p_ptr = p_str;
    while (*p_ptr)
        vga_tm_putc(p_buffer, *p_ptr++);

    return 0;
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

int vga_tm_clear_row(vga_buffer_t* p_buffer, u32 row) {
    if (row >= p_buffer->size.height)
        return 1;

    volatile u16* vga_tm_mem = (volatile u16*)p_buffer->buffer;

    for (u32 c = 0; c < p_buffer->size.width; c++)
        vga_tm_mem[c + p_buffer->size.width * row] = ' ' | (p_buffer->color.color << 8);

    const int result = vga_tm_set_cursor(p_buffer, 0, row);
    return result == 0 ? 0 : 2;
}

int vga_tm_clear_buffer(vga_buffer_t* p_buffer) {
    for (u32 r = 0; r < p_buffer->size.height; r++)
        (void)vga_tm_clear_row(p_buffer, r);

    return vga_tm_set_cursor(p_buffer, 0, 0);
}

int vga_tm_set_cursor(vga_buffer_t* p_buffer, u32 x, u32 y) {
    if (x > p_buffer->size.width || y > p_buffer->size.height)
        return 1;

    int position = x + p_buffer->size.width * y;

    amd64_out_port8(VGA_CRTC_INDEX, 14);
    amd64_out_port8(VGA_CRTC_DATA, (position >> 8) & 0xFF);
    amd64_out_port8(VGA_CRTC_INDEX, 15);
    amd64_out_port8(VGA_CRTC_DATA, position & 0xFF);

    p_buffer->cursor.x = x;
    p_buffer->cursor.y = y;

    return 0;
}

int vga_tm_get_cursor(vga_buffer_t* p_buffer, u32* p_x, u32* p_y) {
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

void vga_gm_set_palette_color(u8 color_index, u8 red, u8 green, u8 blue) {
    amd64_out_port8(0x03C8, color_index);
    amd64_out_port8(0x03C9, red >> 2);
    amd64_out_port8(0x03C9, green >> 2);
    amd64_out_port8(0x03C9, blue >> 2);
}

void vga_gm_startup(vga_buffer_t* p_back_buffer) {
    amd64_out_port8(VGA_MISC_PORT, 0x63);

    amd64_out_port8(VGA_SEQ_INDEX, 0x00); amd64_out_port8(VGA_SEQ_DATA, 0x03);
    amd64_out_port8(VGA_SEQ_INDEX, 0x01); amd64_out_port8(VGA_SEQ_DATA, 0x01);
    amd64_out_port8(VGA_SEQ_INDEX, 0x02); amd64_out_port8(VGA_SEQ_DATA, 0x0F);
    amd64_out_port8(VGA_SEQ_INDEX, 0x03); amd64_out_port8(VGA_SEQ_DATA, 0x00);
    amd64_out_port8(VGA_SEQ_INDEX, 0x04); amd64_out_port8(VGA_SEQ_DATA, 0x0E);

    amd64_out_port8(VGA_CRTC_INDEX, 0x11); amd64_out_port8(VGA_CRTC_DATA, 0x00);

    static const u8 s_crtc_320x200[25] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF
    };

    for (u8 i = 0; i < ARRAY_SIZE(s_crtc_320x200); i++) {
        amd64_out_port8(VGA_CRTC_INDEX, i);
        amd64_out_port8(VGA_CRTC_DATA, s_crtc_320x200[i]);
    }

    amd64_out_port8(VGA_GC_INDEX, 0x00); amd64_out_port8(VGA_GC_DATA, 0x00);
    amd64_out_port8(VGA_GC_INDEX, 0x01); amd64_out_port8(VGA_GC_DATA, 0x00);
    amd64_out_port8(VGA_GC_INDEX, 0x02); amd64_out_port8(VGA_GC_DATA, 0x00);
    amd64_out_port8(VGA_GC_INDEX, 0x03); amd64_out_port8(VGA_GC_DATA, 0x00);
    amd64_out_port8(VGA_GC_INDEX, 0x04); amd64_out_port8(VGA_GC_DATA, 0x00);
    amd64_out_port8(VGA_GC_INDEX, 0x05); amd64_out_port8(VGA_GC_DATA, 0x40);
    amd64_out_port8(VGA_GC_INDEX, 0x06); amd64_out_port8(VGA_GC_DATA, 0x05);
    amd64_out_port8(VGA_GC_INDEX, 0x07); amd64_out_port8(VGA_GC_DATA, 0x0F);
    amd64_out_port8(VGA_GC_INDEX, 0x08); amd64_out_port8(VGA_GC_DATA, 0xFF);

    for (u8 i = 0; i < 16; i++) {
        amd64_in_port8(VGA_INSTAT_READ);
        amd64_out_port8(VGA_AC_INDEX, i);
        amd64_out_port8(VGA_AC_WRITE, i);
    }

    amd64_in_port8(VGA_INSTAT_READ); amd64_out_port8(VGA_AC_INDEX, 0x10); amd64_out_port8(VGA_AC_WRITE, 0x41);
    amd64_in_port8(VGA_INSTAT_READ); amd64_out_port8(VGA_AC_INDEX, 0x11); amd64_out_port8(VGA_AC_WRITE, 0x00);
    amd64_in_port8(VGA_INSTAT_READ); amd64_out_port8(VGA_AC_INDEX, 0x12); amd64_out_port8(VGA_AC_WRITE, 0x0F);
    amd64_in_port8(VGA_INSTAT_READ); amd64_out_port8(VGA_AC_INDEX, 0x13); amd64_out_port8(VGA_AC_WRITE, 0x00);
    amd64_in_port8(VGA_INSTAT_READ); amd64_out_port8(VGA_AC_INDEX, 0x14); amd64_out_port8(VGA_AC_WRITE, 0x00);

    amd64_in_port8(VGA_INSTAT_READ);
    amd64_out_port8(VGA_AC_INDEX, 0x20);

    static const u8 s_ega16[16][3] = {
        {  0,  0,  0}, {  0,  0,170}, {  0,170,  0}, {  0,170,170},
        {170,  0,  0}, {170,  0,170}, {170, 85,  0}, {170,170,170},
        { 85, 85, 85}, { 85, 85,255}, { 85,255, 85}, { 85,255,255},
        {255, 85, 85}, {255, 85,255}, {255,255, 85}, {255,255,255}
    };

    for (u8 i = 0; i < 16; i++)
        vga_gm_set_palette_color(i, s_ega16[i][0], s_ega16[i][1], s_ega16[i][2]);

    memzero((void*)VGA_GM_BUFFER_ADDR, VGA_GM_BUFFER_WIDTH * VGA_GM_BUFFER_HEIGHT * sizeof(u8));
    g_vga_gm_buffer = const_cast<vga_buffer_t*>(p_back_buffer);
}

bool vga_gm_buffer_create(vga_buffer_t* p_back_buffer) {
    if (!p_back_buffer)
        return false;

    constexpr size_t buffer_size = sizeof(u8) * VGA_GM_BUFFER_HEIGHT * VGA_GM_BUFFER_WIDTH;
    p_back_buffer->buffer = (u8*)malloc(buffer_size);

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

    memcpy((void*)VGA_GM_BUFFER_ADDR, (void*)g_vga_gm_buffer->buffer, g_vga_gm_buffer->size.width * g_vga_gm_buffer->size.height * sizeof(u8));

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

bool vga_gm_draw::pixel(vga_buffer_t* p_back_buffer, u64 x, u64 y, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer))
        return false;
    
    if (x >= VGA_GM_BUFFER_WIDTH || y >= VGA_GM_BUFFER_HEIGHT)
        return false;

    p_back_buffer->buffer[y * VGA_GM_BUFFER_WIDTH + x] = (u8)color_index;

    return true;
}

bool vga_gm_draw::lineh(vga_buffer_t* p_back_buffer, u64 x, u64 y, size_t len, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer) || x >= VGA_GM_BUFFER_WIDTH || y >= VGA_GM_BUFFER_HEIGHT)
        return false;

    for (size_t i = x; i < x + len && i < VGA_GM_BUFFER_WIDTH; i++)
        p_back_buffer->buffer[y * VGA_GM_BUFFER_WIDTH + i] = (u8)color_index;

    return true;
}

bool vga_gm_draw::linev(vga_buffer_t* p_back_buffer, u64 x, u64 y, size_t len, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer) || x >= VGA_GM_BUFFER_WIDTH || y >= VGA_GM_BUFFER_HEIGHT)
        return false;

    for (size_t i = y; i < y + len && i < VGA_GM_BUFFER_HEIGHT; i++)
        p_back_buffer->buffer[i * VGA_GM_BUFFER_WIDTH + x] = (u8)color_index;

    return true;
}

bool vga_gm_draw::square(vga_buffer_t* p_back_buffer, u64 x, u64 y, size_t w, size_t h, vga_gm_color_index_t color_index) {
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
            p_back_buffer->buffer[j * VGA_GM_BUFFER_WIDTH + i] = (u8)color_index;
        }
    }

    return true;
}

bool vga_gm_draw::move_square(vga_buffer_t* p_back_buffer, u64 x, u64 y, size_t w, size_t h, size_t nx, size_t ny) {
    if (!IS_VALID_BUFFER(p_back_buffer))
        return false;

    if (x + w > VGA_GM_BUFFER_WIDTH ||
        y + h > VGA_GM_BUFFER_HEIGHT ||
        nx + w > VGA_GM_BUFFER_WIDTH ||
        ny + h > VGA_GM_BUFFER_HEIGHT)
        return false;

    u8* temp = (u8*)malloc(w * h);
    if (!temp)
        return false;

    memcpy(temp, &p_back_buffer->buffer[y * VGA_GM_BUFFER_WIDTH + x], w * h);
    memset(&p_back_buffer->buffer[y * VGA_GM_BUFFER_WIDTH + x], (u8)vga_gm_color_index_t::BLACK, w * h);
    memcpy(&p_back_buffer->buffer[ny * VGA_GM_BUFFER_WIDTH + nx], temp, w * h);

    return true;
}

bool vga_gm_draw::clear(vga_buffer_t* p_back_buffer, vga_gm_color_index_t color_index) {
    if (!IS_VALID_BUFFER(p_back_buffer))
        return false;
    
    memset(p_back_buffer->buffer, (u8)color_index, sizeof(u8) * p_back_buffer->size.width * p_back_buffer->size.height);
    return true;
}