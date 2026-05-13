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
    ARGB888,
    RGBA888,
    BGRA888,
    ABGR888,
};

struct framebuffer_t {
    uint32_t* address;
    size_t size;

    size_t width;
    size_t height;

    size_t pitch;
    size_t caluclated_pitch;

    framebuffer_color_format_t format;
};

uint32_t framebuffer_format_color(framebuffer_t* framebuffer, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

bool framebuffer_init(framebuffer_t* framebuffer, framebuffer_color_format_t format, uint32_t* address, size_t size, size_t width, size_t height, size_t pitch);
bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
bool framebuffer_write_pixel(framebuffer_t* framebuffer, size_t x, size_t y, uint32_t color);

bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
bool framebuffer_write_lineh(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint32_t color);

bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
bool framebuffer_write_linev(framebuffer_t* framebuffer, size_t x, size_t y, size_t len, uint32_t color);

bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
bool framebuffer_write_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, uint32_t color);

bool framebuffer_move_square(framebuffer_t* framebuffer, size_t x, size_t y, size_t w, size_t h, size_t nx, size_t ny);

#endif // __FRAMEBUFFER_HPP__