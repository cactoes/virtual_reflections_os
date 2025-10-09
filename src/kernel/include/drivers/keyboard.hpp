//==========================================
/// @file       keyboard.hpp
/// @brief      generalised keyboard i/o
/// TODO        refactor keyboard_initialize, subscribe_on_key_down
///             & the rest of this entire generic virtual keybaord driver
//==========================================

#pragma once

#ifndef __DRIVERS_KEYBOARD_HPP__
#define __DRIVERS_KEYBOARD_HPP__

#include "common.hpp"
#include "utils/event.hpp"

enum virtual_key_t {
    VK_NONE = 0,

    VK_A, VK_B, VK_C, VK_D, VK_E, VK_F, VK_G, VK_H, VK_I, VK_J,
    VK_K, VK_L, VK_M, VK_N, VK_O, VK_P, VK_Q, VK_R, VK_S, VK_T,
    VK_U, VK_V, VK_W, VK_X, VK_Y, VK_Z,

    VK_0, VK_1, VK_2, VK_3, VK_4,
    VK_5, VK_6, VK_7, VK_8, VK_9,

    VK_SPACE,
    VK_ENTER,
    VK_TAB,
    VK_BACKSPACE,
    VK_ESCAPE,

    VK_MINUS,
    VK_EQUALS,
    VK_LEFT_BRACKET,
    VK_RIGHT_BRACKET,
    VK_BACKSLASH,
    VK_SEMICOLON,
    VK_APOSTROPHE,
    VK_GRAVE,
    VK_COMMA,
    VK_PERIOD,
    VK_SLASH,

    VK_SHIFT,
    VK_CTRL,
    VK_ALT,
    VK_CAPSLOCK,

    VK_UP,
    VK_DOWN,
    VK_LEFT,
    VK_RIGHT,
    VK_HOME,
    VK_END,
    VK_PAGE_UP,
    VK_PAGE_DOWN,
    VK_INSERT,
    VK_DELETE,

    VK_F1,
    VK_F2,
    VK_F3,
    VK_F4,
    VK_F5,
    VK_F6,
    VK_F7,
    VK_F8,
    VK_F9,
    VK_F10,
    VK_F11,
    VK_F12,

    VK_NUMPAD0,
    VK_NUMPAD1,
    VK_NUMPAD2,
    VK_NUMPAD3,
    VK_NUMPAD4,
    VK_NUMPAD5,
    VK_NUMPAD6,
    VK_NUMPAD7,
    VK_NUMPAD8,
    VK_NUMPAD9,
    VK_NUMPAD_PLUS,
    VK_NUMPAD_MINUS,
    VK_NUMPAD_MULTIPLY,
    VK_NUMPAD_DIVIDE,
    VK_NUMPAD_ENTER,
    VK_NUMPAD_PERIOD
};

void keyboard_initialize();

bool holding_shift();
bool holding_caps();
char vk_to_ascii(virtual_key_t vk, bool shift, bool caps);
virtual_key_t wait_for_key();
void subscribe_on_key_down(void(*callback)(virtual_key_t vk));

#endif // __DRIVERS_KEYBOARD_HPP__