#pragma once

#ifndef VROS_WINDOW_HPP
#define VROS_WINDOW_HPP

#include "common.hpp"

typedef u64 window_handle_t;

struct rect_t {
    i32 x, y, w, h;
};

enum window_event_type_t {
    WE_MBL_DOWN = 0,
    WE_MBL_UP,
    WE_MBR_DOWN,
    WE_MBR_UP,
};

struct window_event_t {
    window_event_type_t type;
    union {
        struct { i32 x, y; } mouse;
    };
};

typedef void(*event_hook_t)(window_handle_t handle, window_event_t event);

struct window_desc_t {
    event_hook_t event_hook;

    // x, y are currently unused
    rect_t rect;
};

window_handle_t syscall_create_window(window_desc_t* wnd_desc);
void* syscall_get_window_buffer(window_handle_t handle);
bool syscall_render_window(window_handle_t handle);
bool syscall_poll_event(window_handle_t handle, window_event_t* event, event_hook_t* hook);

#endif // VROS_WINDOW_HPP