#include "drivers/vga_driver.hpp"
#include "string.hpp"
#include "cpu.hpp"
#include "memory.hpp"

static vga_tm_color_map_t g_vga_tm_current_color {
    .color = (uint8_t)vga_tm_color_t::WHITE | ((uint8_t)vga_tm_color_t::BLACK << 4)
};

static uint32_t g_vga_tm_column = 0;
static uint32_t g_vga_tm_row = 0;

static vga_generic_buffer_t* g_back_buffer = nullptr;

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
    size_t strlen = sprintf(buffer, (unsigned long int)sizeof(buffer), fmt, args);
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
    size_t strlen = sprintf(buffer, (unsigned long int)sizeof(buffer), fmt, args);
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
    if (x > VGA_TM_NUM_COLS || y > VGA_TM_NUM_ROWS)
        return 1;

    int position = x + VGA_TM_NUM_COLS * y;

    cpu_outb(VGA_CRTC_INDEX, 14);
    cpu_outb(VGA_CRTC_DATA, (position >> 8) & 0xFF);
    cpu_outb(VGA_CRTC_INDEX, 15);
    cpu_outb(VGA_CRTC_DATA, position & 0xFF);

    g_vga_tm_column = x;
    g_vga_tm_row = y;

    return 0;
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

void vga_gm_startup(const vga_generic_buffer_t* back_buffer) {
    // setup vga in 640x480

    cpu_outb(VGA_MISC_PORT, 0xE3);
    cpu_outb(VGA_SEQ_INDEX, 0x00); cpu_outb(VGA_SEQ_DATA, 0x03);
    cpu_outb(VGA_SEQ_INDEX, 0x01); cpu_outb(VGA_SEQ_DATA, 0x01);
    cpu_outb(VGA_SEQ_INDEX, 0x02); cpu_outb(VGA_SEQ_DATA, 0x0F);
    cpu_outb(VGA_SEQ_INDEX, 0x03); cpu_outb(VGA_SEQ_DATA, 0x00);
    cpu_outb(VGA_SEQ_INDEX, 0x04); cpu_outb(VGA_SEQ_DATA, 0x06);

    cpu_outb(VGA_CRTC_INDEX, 0x11); cpu_outb(VGA_CRTC_DATA, 0x00);
    uint8_t crtc_values[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96,
        0xB9, 0xA3, 0xFF
    };

    for (uint8_t i = 0; i < sizeof(crtc_values); i++) {
        cpu_outb(VGA_CRTC_INDEX, i);
        cpu_outb(VGA_CRTC_DATA, crtc_values[i]);
    }

    cpu_outb(VGA_GC_INDEX, 0x06); cpu_outb(VGA_GC_DATA, 0x05);
    cpu_outb(VGA_GC_INDEX, 0x05); cpu_outb(VGA_GC_DATA, 0x40);
    cpu_outb(VGA_GC_INDEX, 0x04); cpu_outb(VGA_GC_DATA, 0x00);
    
    g_back_buffer = const_cast<vga_generic_buffer_t*>(back_buffer);
}

bool vga_gm_buffer_create(vga_generic_buffer_t* back_buffer) {
    if (!back_buffer)
        return false;

    constexpr size_t buffer_size = sizeof(uint8_t) * VGA_BUFFER_HEIGHT * VGA_BUFFER_WIDTH;
    back_buffer->buffer = (uint8_t*)heap_alloc(get_global_heap(), buffer_size);
    memzero(back_buffer->buffer, buffer_size);

    return true;
}

void vga_gm_buffer_destroy(vga_generic_buffer_t* back_buffer) {
    heap_free(get_global_heap(), back_buffer->buffer);
    back_buffer->buffer = nullptr;
    back_buffer->size = 0;
}

bool vga_gm_render() {
    if (!g_back_buffer || !g_back_buffer->buffer)
        return false;

    memcpy((void*)VGA_BUFFER_ADDR, (void*)g_back_buffer->buffer, g_back_buffer->size);

    return true;
}

bool vga_gm_swap_back_buffer(vga_generic_buffer_t** back_buffer_new, vga_generic_buffer_t** back_buffer_old) {
    if (!back_buffer_new || !g_back_buffer)
        return false;

    if (back_buffer_old)
        *back_buffer_old = g_back_buffer;

    g_back_buffer = *back_buffer_new;

    return true;
}

bool vga_gm_draw::set_pixel(vga_generic_buffer_t* back_buffer, uint64_t x, uint64_t y, vga_gm_color_index_t color_index) {
    if (!g_back_buffer)
        return false;
    
    if (x > VGA_BUFFER_WIDTH || y > VGA_BUFFER_HEIGHT)
        return false;

    g_back_buffer->buffer[y * VGA_BUFFER_WIDTH + x] = (uint8_t)color_index;

    return true;
}

/*

// Function to set a color in the palette
void set_palette_color(enum VGAColor color, uint8_t red, uint8_t green, uint8_t blue) {
    // Set the palette index using the VGA index register (0x03C8)
    outb(0x03C8, color);
    
    // Set the RGB values (6 bits per channel, range 0-63)
    outb(0x03C9, red >> 2);   // Red
    outb(0x03C9, green >> 2); // Green
    outb(0x03C9, blue >> 2);  // Blue
}

*/