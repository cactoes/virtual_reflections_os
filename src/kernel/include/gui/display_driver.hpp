//==========================================
/// @file       display_driver.hpp
/// @brief      display driver + kernel window manager
//==========================================

#pragma once

#ifndef __DISPLAY_DRIVER_HPP__
#define __DISPLAY_DRIVER_HPP__

#define WINDOW_HANDLE_INVALID MAX_UINT64

#include "common.hpp"
#include "std/ring_buffer.hpp"
#include "process.hpp"
#include "vrosapi/window.hpp"

typedef u64 window_handle_t;

struct window_t {
    int width, height;
    int x, y;
    void* buffer;
    bool is_dragging;
    process_t* parent_process;
    window_handle_t handle;
    bool active;

    event_hook_t event_hook;
    std::ring_buffer<8, window_event_t> event_queue;
};

struct cursor_t {
    u64 x, y;
};

enum class kdc_action_t {
    MMOVE = 0,
    MDOWNL,
    MUPL,
    MDOWNR,
    MUPR
};

struct window_manager_t {
    bool should_render;
    std::dynamic_array<window_t*> windows;
    cursor_t cursor;
};

void dd_set_active_buffer(void* buffer);
int dd_buffer_render_loop();

bool wm_init(window_manager_t* wm);
void set_global_window_manager(window_manager_t* wm);
window_manager_t* get_global_window_manager();

window_t* wm_create_window(int w, int h, int x, int y);
window_handle_t wm_allocate_window(window_manager_t* wm, int w, int h, event_hook_t hook);

void* wm_window_get_buffer(window_manager_t* wm, window_handle_t handle);
bool wm_window_poll_event(window_manager_t* wm, window_handle_t handle, window_event_t* event, event_hook_t* hook);
bool wm_window_resize(window_manager_t* wm, window_handle_t handle, u32 w, u32 h);

void wm_handle_mouse_event(window_manager_t* wm, int x, int y, kdc_action_t action);

int wm_render_loop();

#endif // __DISPLAY_DRIVER_HPP__