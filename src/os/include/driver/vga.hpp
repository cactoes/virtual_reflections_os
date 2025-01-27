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

namespace kernel::driver::vga {

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