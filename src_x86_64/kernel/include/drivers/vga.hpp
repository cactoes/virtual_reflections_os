//==========================================
/// @file       vga.hpp
/// @brief      vga text driver
//==========================================

#pragma once

#ifndef __DRIVERS_VGA_HPP__
#define __DRIVERS_VGA_HPP__

#define VGA_CRTC_INDEX      0x03D4
#define VGA_CRTC_DATA       0x03D5

#define VGA_AC_INDEX        0x3C0
#define VGA_AC_WRITE        0x3C0
#define VGA_MISC_PORT       0x03C2
#define VGA_SEQ_INDEX       0x03C4
#define VGA_SEQ_DATA        0x03C5
#define VGA_GC_INDEX        0x03CE
#define VGA_GC_DATA         0x03CF
#define VGA_INSTAT_READ     0x03DA

#define VGA_TM_BUFFER_ADDR  PTOV_I(0xB8000)
#define VGA_TM_NUM_COLS     80
#define VGA_TM_NUM_ROWS     25

#define VGA_GM_BUFFER_ADDR     PTOV_I(0xA0000)
#define VGA_GM_BUFFER_WIDTH    320
#define VGA_GM_BUFFER_HEIGHT   200

#define IS_VALID_BUFFER(buff) ((buff) && (buff)->buffer && (buff)->size.width == VGA_GM_BUFFER_WIDTH && (buff)->size.height == VGA_GM_BUFFER_HEIGHT)

#include "common.hpp"
#include "linker.hpp"

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

enum class vga_gm_color_index_t : uint8_t {
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

struct vga_buffer_t {
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

extern vga_buffer_t g_vga_tm_buffer;

void vga_tm_init_buffer(vga_buffer_t* p_buffer, void* p_vga_array, size_t width, size_t height);
int vga_tm_putc(vga_buffer_t* p_buffer, char ch);
int vga_tm_puts(vga_buffer_t* p_buffer, const char* p_str);
int vga_tm_puts_color(vga_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map, const char* p_str);
int vga_tm_clear_row(vga_buffer_t* p_buffer, uint32_t row);
int vga_tm_clear_buffer(vga_buffer_t* p_buffer);
int vga_tm_set_cursor(vga_buffer_t* p_buffer, uint32_t x, uint32_t y);
int vga_tm_get_cursor(vga_buffer_t* p_buffer, uint32_t* p_x, uint32_t* p_y);
int vga_tm_set_color(vga_buffer_t* p_buffer, const vga_tm_color_map_t* p_color_map);
int vga_tm_get_color(vga_buffer_t* p_buffer, vga_tm_color_map_t* p_color_map);

void vga_gm_set_palette_color(uint8_t color_index, uint8_t red, uint8_t green, uint8_t blue);
void vga_gm_startup(vga_buffer_t* p_back_buffer);
bool vga_gm_buffer_create(vga_buffer_t* p_back_buffer);
void vga_gm_buffer_destroy(vga_buffer_t* p_back_buffer);
bool vga_gm_render();
bool vga_gm_swap_back_buffer(vga_buffer_t** p_back_buffer_new, vga_buffer_t** p_back_buffer_old);

namespace vga_gm_draw {
    bool pixel(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, vga_gm_color_index_t color_index);
    bool lineh(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, size_t len, vga_gm_color_index_t color_index);
    bool linev(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, size_t len, vga_gm_color_index_t color_index);
    bool square(vga_buffer_t* p_back_buffer, uint64_t x, uint64_t y, size_t w, size_t h, vga_gm_color_index_t color_index);
    bool clear(vga_buffer_t* p_back_buffer, vga_gm_color_index_t color_index);
} // namespace 

#endif // __DRIVERS_VGA_HPP__