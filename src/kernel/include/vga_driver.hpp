//==========================================
/// @file       vga_driver.hpp
/// @brief      vga related functions
//==========================================

#pragma once

#ifndef __VGA_DRIVER_HPP__
#define __VGA_DRIVER_HPP__

#define VGA_MISC_PORT       0x03C2
#define VGA_SEQ_INDEX       0x03C4
#define VGA_SEQ_DATA        0x03C5
#define VGA_GC_INDEX        0x03CE
#define VGA_GC_DATA         0x03CF
#define VGA_CRTC_INDEX      0x03D4
#define VGA_CRTC_DATA       0x03D5

#define VGA_BUFFER_ADDR     0xA0000
#define VGA_BUFFER_WIDTH    640
#define VGA_BUFFER_HEIGHT   480

#define VGA_TM_BUFFER_ADDR  0xB8000
#define VGA_TM_NUM_COLS     80
#define VGA_TM_NUM_ROWS     25

#include "common.hpp"

enum class vga_gm_color_index_t : uint8_t {
    BLACK =         0x00,   // RGB: (0, 0, 0)
    BLUE =          0x01,   // RGB: (0, 0, 63)
    GREEN =         0x02,   // RGB: (0, 63, 0)
    CYAN =          0x03,   // RGB: (0, 63, 63)
    RED =           0x04,   // RGB: (63, 0, 0)
    MAGENTA =       0x05,   // RGB: (63, 0, 63)
    BROWN =         0x06,   // RGB: (63, 31, 0)
    LIGHT_GRAY =    0x07,   // RGB: (95, 95, 95)
    DARK_GRAY =     0x08,   // RGB: (47, 47, 47)
    LIGHT_BLUE =    0x09,   // RGB: (47, 47, 95)
    LIGHT_GREEN =   0x0A,   // RGB: (47, 95, 47)
    LIGHT_CYAN =    0x0B,   // RGB: (47, 95, 95)
    LIGHT_RED =     0x0C,   // RGB: (95, 47, 47)
    LIGHT_MAGENTA = 0x0D,   // RGB: (95, 47, 95)
    YELLOW =        0x0E,   // RGB: (95, 95, 47)
    WHITE =         0x0F,   // RGB: (95, 95, 95)
};

enum class vga_tm_color_t : uint8_t {
    BLACK =       0,
    BLUE =        1,
    GREEN =       2,
    CYAN =        3,
    RED =         4,
    MAGENTA =     5,
    BROWN =       6,
    LIGHT_GRAY =  7,
    DARK_GRAY =   8,
    LIGHT_BLUE =  9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN =  11,
    LIGHT_RED =   12,
    PINK =        13,
    YELLOW =      14,
    WHITE =       15,
};

struct vga_tm_color_map_t {
    union {
        struct {
            vga_tm_color_t foreground : 4;
            vga_tm_color_t background : 4;
        };

        uint8_t color;
    };
};

struct vga_generic_buffer_t {
    size_t size;
    uint8_t* buffer;
};

int vga_tm_print(char ch);
int vga_tm_print(const char* fmt, ...);
int vga_tm_print(const vga_tm_color_map_t* color_map, const char* fmt, ...);
int vga_tm_clear_row(uint32_t row);
int vga_tm_clear_screen();
int vga_tm_set_cursor(uint32_t x, uint32_t y);
int vga_tm_get_cursor(uint32_t* x, uint32_t* y);
int vga_tm_set_color(const vga_tm_color_map_t* color_map);
int vga_tm_get_color(vga_tm_color_map_t* color_map);

void vga_gm_startup(const vga_generic_buffer_t* back_buffer);
bool vga_gm_buffer_create(vga_generic_buffer_t* back_buffer);
void vga_gm_buffer_destroy(vga_generic_buffer_t* back_buffer);
bool vga_gm_render();
bool vga_gm_swap_back_buffer(vga_generic_buffer_t** back_buffer_new, vga_generic_buffer_t** back_buffer_old);

namespace vga_gm_draw {
    bool set_pixel(vga_generic_buffer_t* back_buffer, uint64_t x, uint64_t y, vga_gm_color_index_t color_index);
} // namespace 

#endif // __VGA_DRIVER_HPP__