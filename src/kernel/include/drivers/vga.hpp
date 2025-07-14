//==========================================
/// @file       vga.hpp
/// @brief      vga text driver
//==========================================

#pragma once

#ifndef __DRIVERS_VGA_HPP__
#define __DRIVERS_VGA_HPP__

#define VGA_CRTC_INDEX      0x03D4
#define VGA_CRTC_DATA       0x03D5

#define VGA_TM_BUFFER_ADDR  0xB8000
#define VGA_TM_NUM_COLS     80
#define VGA_TM_NUM_ROWS     25

#include "common.hpp"

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

struct vga_tm_buffer_t {
    uint8_t* buffer;
    vga_tm_color_map_t color;

    struct {
        size_t width;
        size_t height;
    } size;
    
    struct {
        size_t x;
        size_t y;
    } cursor;
};

extern vga_tm_buffer_t g_vga_tm_buffer;

void vga_tm_init_buffer(vga_tm_buffer_t* p_buffer, void* p_vga_array, size_t width, size_t height);
int vga_tm_putc(vga_tm_buffer_t* p_buffer, char ch);
int vga_tm_puts(vga_tm_buffer_t* p_buffer, const char* p_str);
int vga_tm_puts_color(vga_tm_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map, const char* p_str);
int vga_tm_clear_row(vga_tm_buffer_t* p_buffer, uint32_t row);
int vga_tm_clear_buffer(vga_tm_buffer_t* p_buffer);
int vga_tm_set_cursor(vga_tm_buffer_t* p_buffer, uint32_t x, uint32_t y);
int vga_tm_get_cursor(vga_tm_buffer_t* p_buffer, uint32_t* p_x, uint32_t* p_y);
int vga_tm_set_color(vga_tm_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map);
int vga_tm_get_color(vga_tm_buffer_t* p_buffer, vga_tm_color_map_t* p_color_map);


#endif // __DRIVERS_VGA_HPP__