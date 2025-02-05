//==========================================
/// @file       vga.hpp
/// @brief      vga related functions
//==========================================

#pragma once

#ifndef __VGA_HPP__
#define __VGA_HPP__

#include "../common.hpp"

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

/// @brief namespace for vga related functions
namespace kernel::driver::vga {

/// @brief namespace for text mode vga functions
namespace tm {

enum vga_color_t : uint8_t {
    VGAC_BLACK = 0,
    VGAC_BLUE = 1,
    VGAC_GREEN = 2,
    VGAC_CYAN = 3,
    VGAC_RED = 4,
    VGAC_MAGENTA = 5,
    VGAC_BROWN = 6,
    VGAC_LIGHT_GRAY = 7,
    VGAC_DARK_GRAY = 8,
    VGAC_LIGHT_BLUE = 9,
    VGAC_LIGHT_GREEN = 10,
    VGAC_LIGHT_CYAN = 11,
    VGAC_LIGHT_RED = 12,
    VGAC_PINK = 13,
    VGAC_YELLOW = 14,
    VGAC_WHITE = 15,
};

typedef struct __KD_VGA_COLOR_MAP {
    union {
        struct {
            vga_color_t foreground : 4;
            vga_color_t background : 4;
        };

        uint8_t color;
    };
} vga_color_map_t;

/// @brief      prints a single chat to the text mode vga screen at the current pos
/// @param ch   char to print
/// @return     KRESULT(0): success
/// @remarks    does not move the cursor
[[nodiscard]] kresult_t
print(
    char ch);

/// @brief          prints a format string to text mode vga screen at current pos
/// @param fmt      string to print w/ formats
/// @param va_args  argument list
/// @return         KRESULT(0): success
///                 KRESULT(1): invalid pointer
///                 KRESULT(2): issue with set_cursor
[[nodiscard]] kresult_t
print(
    const char* fmt,
    ...);

/// @brief              prints a format string to text mode vga screen at current pos
/// @param[in] color    color to set this bit of text to
/// @param fmt          string to print w/ formats
/// @param va_args      argument list
/// @return             KRESULT(0): success
///                     KRESULT(1): invalid pointer
///                     KRESULT(2): issue with set_cursor
[[nodiscard]] kresult_t
print(
    const vga_color_map_t* color_map,
    const char* fmt,
    ...);

/// @brief      sets the vga mode cursor to pos
/// @param x    x pos (column)
/// @param y    y pos (row)
/// @return     KRESULT(0): success
[[nodiscard]] kresult_t
set_cursor(
    uint32_t x,
    uint32_t y);

/// @brief          gets the current cursor pos of the text mode vga cursor
/// @param[out] x   x pos (column)
/// @param[out] y   y pos (row)
/// @return         KRESULT(0): success
///                 KRESULT(1): pointer was invalid
[[nodiscard]] kresult_t
get_cursor(
    uint32_t* x,
    uint32_t* y);

/// @brief      clears the vga buffer
/// @return     KRESULT(0): success
///             KRESULT(1): issue with set_cursor
[[nodiscard]] kresult_t
clear_screen();

/// @brief          clears a row in the vga buffer
/// @param row      row index
/// @return         KRESULT(0): success
///                 KRESULT(1): row index out of bounds
///                 KRESULT(2): issue with set_cursor
[[nodiscard]] kresult_t
clear_row(
    uint32_t row);

/// @brief                  sets the color of the output text
/// @param[in] color_map    text color struct
/// @return                 KRESULT(0): success
///                         KRESULT(1): invalid pointer
[[nodiscard]] kresult_t
set_color(
    const vga_color_map_t* color_map);

/// @brief                  gets the current text color structs
/// @param[out] color_map   pointer to text color struct
/// @return                 KRESULT(0): success
///                         KRESULT(1): invalid pointer
[[nodiscard]] kresult_t
get_color(
    vga_color_map_t* color_map);

} // namespace tm

typedef struct __KD_VGA_BUFFER {
    size_t size;
    uint8_t* buffer;
} vga_buffer_t;

[[nodiscard]] kresult_t
startup_vga_graphics(
    const vga_buffer_t* back_buffer);

[[nodiscard]] kresult_t
vga_buffer_create(
    vga_buffer_t* buffer);

[[nodiscard]] kresult_t
vga_buffer_destroy(
    vga_buffer_t* buffer);

[[nodiscard]] kresult_t
update_vga();

[[nodiscard]] kresult_t
swap_vga_back_buffer(
    vga_buffer_t** back_buffer_new,
    vga_buffer_t** back_buffer_old);

[[nodiscard]] kresult_t
back_buffer_set_pixel(
    uint64_t x,
    uint64_t y,
    uint8_t color_index);

} // namespace kernel::driver::vga

#endif // __VGA_HPP__