//==========================================
/// @file       keyboard.hpp
/// @brief      simple keyboard driver
//==========================================

#pragma once

#ifndef __DRIVER_KEYBOARD_HPP__
#define __DRIVER_KEYBOARD_HPP__

#include "../common.hpp"

#define SC_CAPS_LOCK        0x3A
#define SC_LALT             0x38
#define SC_LSHIFT           0x2A
#define SC_RSHIFT           0x36
#define SC_CTRL             0x1D
#define SC_NUMLOCK          0x45

#define SCAN_CODE_MASK_CHAR          0x7F
#define SCAN_CODE_MASK_PRESSED       0x80
#define SCAN_CODE_ESCAPED            0x60

#define FULL_CODE_ESCAPED            0xE0

#define KEYBOARD_PORT_SCANCODE       0x60

/// @brief namespace for keyboard interaction
namespace kernel::driver::keyboard {

typedef struct __KD_KEYBOARD_STATE {
    bool shift = false;
    bool caps_lock = false;
    bool control = false;
    bool alt = false;
    bool num_lock = false;
} keyboard_state_t;

typedef struct __KD_KEYBOARD_KEY_STATE {
    uint64_t full_code = 0;
    uint64_t scan_code = 0;
    bool is_released = false;
    bool is_escaped = false;
} key_state_t;

/// @brief                              checks if the keypress was a special key or a printable key
/// @param[inout] keyboard_state        keyboard_state struct to update and compare
/// @param[in] key_state                key_state struct to check
/// @return                             true if the key was a special key & handled
/// @remarks                            this is a very simple implementation & can
///                                     cause invalid behaviour
bool
handle_special_key(
    keyboard_state_t* keyboard_state,
    key_state_t* key_state);

/// @brief                  handles keyboard interrupts
/// @param[inout] state     keyboard_state struct to update and compare
/// @remarks                still very incomplete
void
handle_interrupt(
    keyboard_state_t* state);

/// @brief      gets the current state of a key
/// @param ch   input char to test
/// @return     pointer to keystate object
key_state_t*
get_key_state(uint64_t ch);

} // namespace kernel::driver::keyboard

#endif // __DRIVER_KEYBOARD_HPP__