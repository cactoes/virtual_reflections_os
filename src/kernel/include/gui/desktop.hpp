//==========================================
/// @file       desktop.hpp
/// @brief      visual user interace stuff
//==========================================

#pragma once

#ifndef __DESKTOP_HPP__
#define __DESKTOP_HPP__

#define DESKTOP_EVENT_MOUSE_MOVE        "on_mouse_move"
#define DESKTOP_EVENT_MOUSE_SCROLL      "on_mouse_scroll"
#define DESKTOP_EVENT_MOUSE_PRESSED     "on_mouse_pressed"
#define DESKTOP_EVENT_MOUSE_RELEASED    "on_mouse_release"

#include "common.hpp"

struct desktop_render_target_t {
    typedef void(*callback_t)(uint64_t dt, uint64_t x, uint64_t y);
    callback_t callback;
    uint64_t x, y;
    uint64_t w, h;
    bool dragging;
};

struct desktop_event_on_mouse_move_t {
    int x;
    int y;
};

struct desktop_event_on_mouse_scroll_t {
    int d;
};

enum class desktop_event_mouse_button_type_t {
    LEFT,
    MIDDLE,
    RIGHT
};

struct desktop_event_on_mouse_button_t {
    desktop_event_mouse_button_type_t key;
};

struct desktop_render_color_t {
    uint8_t r, g, b;
};

int desktop_init();
bool desktop_register_target(desktop_render_target_t p_target);

bool desktop_render_pixel(int x, int y, const desktop_render_color_t& color);
bool desktop_render_pixel(int x, int y, uint8_t vga_color_index);
bool desktop_render_linev(int x, int y, size_t l, const desktop_render_color_t& color);
bool desktop_render_lineh(int x, int y, size_t l,const desktop_render_color_t& color);
bool desktop_render_square(int x, int y, size_t w, size_t h, const desktop_render_color_t& color);

bool desktop_event_subscribe(const char* p_name, void(*p_callback)(void*));

template <typename T>
bool desktop_event_subscribe(const char* p_name, void(*p_callback)(T*)) {
    return desktop_event_subscribe(p_name, (void(*)(void*))p_callback);
}

void desktop_get_cursor_pos(int* p_x, int* p_y);

bool is_desktop_ready();

#endif // __DESKTOP_HPP__