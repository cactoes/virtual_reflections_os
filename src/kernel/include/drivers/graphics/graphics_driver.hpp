//==========================================
/// @file       graphics_driver.hpp
/// @brief      generic graphics interface
//==========================================

#pragma once

#ifndef __GRAPHICS_DRIVER_HPP__
#define __GRAPHICS_DRIVER_HPP__

#include "common.hpp"
#include "drivers/graphics/framebuffer.hpp"
#include "drivers/vga.hpp"
#include "multiboot.hpp"

enum class graphics_driver_type_t {
    UNKNOWN = 0,
    VGA,
    FRAMEBUFFER
};

struct color_t {
    uint8_t r, g, b;
    uint8_t a = 255;
};

struct graphics_driver_t {
    graphics_driver_type_t type;

    union {
        framebuffer_t* framebuffer;
        vga_buffer_t* vgabuffer;
    };
};

graphics_driver_t* get_global_graphics_driver();
void set_global_graphics_driver(graphics_driver_t* graphics_driver);

framebuffer_color_format_t get_framebuffer_format(multiboot2_tag_framebuffer_t* tag);

bool graphics_driver_init(graphics_driver_t* graphics_driver, multiboot_t* multiboot_struct);

bool graphics_driver_draw_pixel(graphics_driver_t* graphics_driver, size_t x, size_t y, const color_t& color);
bool graphics_driver_draw_lineh(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t len, const color_t& color);
bool graphics_driver_draw_linev(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t len, const color_t& color);
bool graphics_driver_draw_square(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t w, size_t h, const color_t& color);
bool graphics_driver_draw_character(graphics_driver_t* graphics_driver, size_t x, size_t y, char c, const color_t& color, float scale = 1.f);
bool graphics_driver_draw_text(graphics_driver_t* graphics_driver, size_t x, size_t y, const char* text, const color_t& color, float scale = 1.f);
bool graphics_driver_move_square(graphics_driver_t* graphics_driver, size_t x, size_t y, size_t w, size_t h, size_t nx, size_t ny);

#endif // __GRAPHICS_DRIVER_HPP__