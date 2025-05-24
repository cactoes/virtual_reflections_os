//==========================================
/// @file       keyboard_driver.hpp
/// @brief      simple keyboard driver
///  TODO       translate keyboard keys function
//==========================================

#pragma once

#ifndef __KEYBOARD_DRIVER_HPP__
#define __KEYBOARD_DRIVER_HPP__

#define SC_CAPS_LOCK        0x3A
#define SC_LALT             0x38
#define SC_LSHIFT           0x2A
#define SC_RSHIFT           0x36
#define SC_CTRL             0x1D
#define SC_NUMLOCK          0x45

#define SCAN_CODE_MASK_CHAR         0x7F
#define SCAN_CODE_MASK_PRESSED      0x80
#define SCAN_CODE_ESCAPED           0x60

#define FULL_CODE_ESCAPED           0xE0

#define KEYBOARD_PORT_SCANCODE      0x60

#define KEY_STATE_ARRAY_SIZE        0xFF

#include "common.hpp"
#include "interrupt.hpp"
#include "event_manager.hpp"

struct key_state_t {
    uint64_t full_code = 0;
    uint64_t scan_code = 0;
    bool is_released = false;
    bool is_escaped = false;
};

void keyboard_init(key_state_t keystates[KEY_STATE_ARRAY_SIZE]);
key_state_t* keyboard_get_key_state(uint64_t scan_code);
cpu_state_t* keyboard_handle_interrupt(uint64_t code, cpu_state_t* rsp);
void keyboard_add_handler(void(*handler)(key_state_t*));

#endif // __KEYBOARD_DRIVER_HPP__