//==========================================
/// @file       kterminal.hpp
/// @brief      kernel terminal emulator
//==========================================

#pragma once

#ifndef __KTERMINAL_HPP__
#define __KTERMINAL_HPP__

#include "common.hpp"
#include "std/array.hpp"
#include "drivers/graphics/graphics_driver.hpp"

struct kterminal_t {
    u64 width;
    u64 height;

    u64 useable_height;
    u64 useable_width;

    u32 x_rows;
    u32 y_rows;

    // index based
    struct { u32 x, y; } cursor;

    bool is_ready;

    std::dynamic_array<char> current_input {};
    bool keep_alive;

    color_t fg_color { 255, 255, 255 };
};

bool kterm_init(kterminal_t* kterm);
bool kterm_write_stream(kterminal_t* kterm, const char* buffer, u64 size);
bool kterm_start(kterminal_t* kterm);

void set_kterm_session(kterminal_t* kterm);
kterminal_t* get_kterm_session();

#endif // __KTERMINAL_HPP__