//==========================================
/// @file       framebuffer.hpp
/// @brief      framebuffer driver for GOP
//==========================================

#pragma once

#ifndef __FRAMEBUFFER_HPP__
#define __FRAMEBUFFER_HPP__

#include "common.hpp"

enum class framebuffer_color_format_t {
    UNKNOWN = 0,
    ARGB8888,
    RGBA8888,
    BGRA8888,
    ABGR8888,

    RGB888,
};

struct framebuffer_t {
    u32* address;
    u32* back_buffer;
    size_t size;

    size_t width;
    size_t height;

    size_t pitch;
    size_t caluclated_pitch;

    framebuffer_color_format_t format;
};

u32 framebuffer_format_color(framebuffer_t* framebuffer, u8 r, u8 g, u8 b, u8 a);

bool framebuffer_init(framebuffer_t* framebuffer, framebuffer_color_format_t format, u32* address, size_t size, size_t width, size_t height, size_t pitch);
bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, u8 r, u8 g, u8 b, u8 a = 255);
bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, u32 color);
void framebuffer_write_pixel_raw(framebuffer_t* framebuffer, size_t x, size_t y, u32 color);

bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u8 r, u8 g, u8 b, u8 a = 255);
bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u32 color);

bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u8 r, u8 g, u8 b, u8 a = 255);
bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, u32 color);

bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, u8 r, u8 g, u8 b, u8 a = 255);
bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, u32 color);

bool framebuffer_move_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, size_t nx, size_t ny);

bool framebuffer_render(framebuffer_t* framebuffer);

#endif // __FRAMEBUFFER_HPP__