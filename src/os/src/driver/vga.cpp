#define __KERNEL_MEMORY_FULL__

#include "driver/vga.hpp"
#include "critical/kernel.hpp"
#include "critical/memory.hpp"

kernel::driver::vga::vga_buffer_t* g_back_buffer = nullptr;

kresult_t kernel::driver::vga::startup_vga_graphics(const vga_buffer_t* back_buffer) {
    // setup vga in 640x480

    (void)cpu::out_port(cpu::PT_B, VGA_MISC_PORT, 0xE3);

    (void)cpu::out_port(cpu::PT_B, VGA_SEQ_INDEX, 0x00); (void)cpu::out_port(cpu::PT_B, VGA_SEQ_DATA, 0x03);
    (void)cpu::out_port(cpu::PT_B, VGA_SEQ_INDEX, 0x01); (void)cpu::out_port(cpu::PT_B, VGA_SEQ_DATA, 0x01);
    (void)cpu::out_port(cpu::PT_B, VGA_SEQ_INDEX, 0x02); (void)cpu::out_port(cpu::PT_B, VGA_SEQ_DATA, 0x0F);
    (void)cpu::out_port(cpu::PT_B, VGA_SEQ_INDEX, 0x03); (void)cpu::out_port(cpu::PT_B, VGA_SEQ_DATA, 0x00);
    (void)cpu::out_port(cpu::PT_B, VGA_SEQ_INDEX, 0x04); (void)cpu::out_port(cpu::PT_B, VGA_SEQ_DATA, 0x06);

    (void)cpu::out_port(cpu::PT_B, VGA_CRTC_INDEX, 0x11); (void)cpu::out_port(cpu::PT_B, VGA_CRTC_DATA, 0x00);
    uint8_t crtc_values[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96,
        0xB9, 0xA3, 0xFF
    };

    for (uint8_t i = 0; i < sizeof(crtc_values); i++) {
        (void)cpu::out_port(cpu::PT_B, VGA_CRTC_INDEX, i);
        (void)cpu::out_port(cpu::PT_B, VGA_CRTC_DATA, crtc_values[i]);
    }

    (void)cpu::out_port(cpu::PT_B, VGA_GC_INDEX, 0x06); (void)cpu::out_port(cpu::PT_B, VGA_GC_DATA, 0x05);
    (void)cpu::out_port(cpu::PT_B, VGA_GC_INDEX, 0x05); (void)cpu::out_port(cpu::PT_B, VGA_GC_DATA, 0x40);
    (void)cpu::out_port(cpu::PT_B, VGA_GC_INDEX, 0x04); (void)cpu::out_port(cpu::PT_B, VGA_GC_DATA, 0x00);
    
    g_back_buffer = const_cast<vga_buffer_t*>(back_buffer);

    return KRESULT(0);
}

kresult_t kernel::driver::vga::vga_buffer_create(vga_buffer_t* buffer) {
    if (!buffer)
        return KRESULT(1);

    constexpr size_t buffer_size = sizeof(uint8_t) * VGA_BUFFER_HEIGHT * VGA_BUFFER_WIDTH;
    buffer->buffer = (uint8_t*)memory::vmem::kalloc(buffer_size);
    memzero(buffer->buffer, buffer_size);

    buffer->size = buffer_size;
    return KRESULT(0);
}

kresult_t kernel::driver::vga::vga_buffer_destroy(vga_buffer_t* buffer) {
    memory::vmem::kfree(buffer->buffer);
    buffer->buffer = nullptr;
    buffer->size = 0;
    return KRESULT(0);
}

kresult_t kernel::driver::vga::update_vga() {
    if (!g_back_buffer || !g_back_buffer->buffer)
        return KRESULT(1);

    memcpy((void*)VGA_BUFFER_ADDR, (void*)g_back_buffer->buffer, g_back_buffer->size);

    return KRESULT(0);
}

kresult_t kernel::driver::vga::swap_vga_back_buffer(vga_buffer_t** back_buffer_new, vga_buffer_t** back_buffer_old) {
    if (!g_back_buffer)
        return KRESULT(1);

    if (!back_buffer_new)
        return KRESULT(2);

    if (back_buffer_old)
        *back_buffer_old = g_back_buffer;

    g_back_buffer = *back_buffer_new;

    return KRESULT(0);
}

kresult_t kernel::driver::vga::back_buffer_set_pixel(uint64_t x, uint64_t y, uint8_t color_index) {
    if (!g_back_buffer)
        return KRESULT(1);

    if (x > VGA_BUFFER_WIDTH || y > VGA_BUFFER_HEIGHT)
        return KRESULT(2);

    g_back_buffer->buffer[y * VGA_BUFFER_WIDTH + x] = color_index;

    return KRESULT(0);
}

/*

// VGA color indexes
enum VGAColor {
    VGA_COLOR_BLACK = 0x00,         // RGB: (0, 0, 0)
    VGA_COLOR_BLUE = 0x01,          // RGB: (0, 0, 63)
    VGA_COLOR_GREEN = 0x02,         // RGB: (0, 63, 0)
    VGA_COLOR_CYAN = 0x03,          // RGB: (0, 63, 63)
    VGA_COLOR_RED = 0x04,           // RGB: (63, 0, 0)
    VGA_COLOR_MAGENTA = 0x05,       // RGB: (63, 0, 63)
    VGA_COLOR_BROWN = 0x06,         // RGB: (63, 31, 0)
    VGA_COLOR_LIGHT_GRAY = 0x07,    // RGB: (95, 95, 95)
    VGA_COLOR_DARK_GRAY = 0x08,     // RGB: (47, 47, 47)
    VGA_COLOR_LIGHT_BLUE = 0x09,    // RGB: (47, 47, 95)
    VGA_COLOR_LIGHT_GREEN = 0x0A,   // RGB: (47, 95, 47)
    VGA_COLOR_LIGHT_CYAN = 0x0B,    // RGB: (47, 95, 95)
    VGA_COLOR_LIGHT_RED = 0x0C,     // RGB: (95, 47, 47)
    VGA_COLOR_LIGHT_MAGENTA = 0x0D, // RGB: (95, 47, 95)
    VGA_COLOR_YELLOW = 0x0E,        // RGB: (95, 95, 47)
    VGA_COLOR_WHITE = 0x0F,         // RGB: (95, 95, 95)

    // Additional custom colors (can be expanded as needed)
    VGA_COLOR_GRAY = 0x10,          // RGB: (80, 80, 80)
    VGA_COLOR_LIGHT_YELLOW = 0x11,  // RGB: (95, 95, 63)
    VGA_COLOR_ORANGE = 0x12,        // RGB: (95, 63, 0)
    VGA_COLOR_PURPLE = 0x13,        // RGB: (63, 0, 63)
    // ... You can continue with more custom colors
};

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