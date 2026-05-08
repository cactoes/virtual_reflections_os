//==========================================
/// @file       keyboard.hpp
/// @brief      ps2 keyboard driver
//==========================================

#pragma once

#ifndef __DRIVERS_PS2_KEYBOARD_HPP__
#define __DRIVERS_PS2_KEYBOARD_HPP__

#define PS2_KEYBOARD_SC_CAPS_LOCK        0x3A
#define PS2_KEYBOARD_SC_LALT             0x38
#define PS2_KEYBOARD_SC_LSHIFT           0x2A
#define PS2_KEYBOARD_SC_RSHIFT           0x36
#define PS2_KEYBOARD_SC_CTRL             0x1D
#define PS2_KEYBOARD_SC_NUMLOCK          0x45

#define PS2_KEYBOARD_SCAN_CODE_MASK_CHAR         0x7F
#define PS2_KEYBOARD_SCAN_CODE_MASK_PRESSED      0x80

#define PS2_KEYBOARD_FULL_CODE_ESCAPED           0xE0

#define PS2_KEYBOARD_KEY_STATE_ARRAY_SIZE        128

#include "common.hpp"
#include "cpu.hpp"

struct ps2_key_state_t {
    uint32_t full_code = 0;
    uint32_t scan_code = 0;
    struct {
        bool is_released : 1;
        bool is_pressed : 1;
        bool is_escaped : 1;
        bool is_shift : 1;
        bool is_capslock : 1;
    };
} PACKED;

interrupt_regs_t* ps2_keyboard_handle_interrupt(interrupt_regs_t* p_rsp, void*);
uint32_t ps2_keyboard_get_last_scancode();
void ps2_keyboard_clear_last_scancode();
const ps2_key_state_t* ps2_keyboard_get_key_state(uint32_t scan_code);
bool ps2_keyboard_is_scan_code_extended(uint32_t scan_code);
void ps2_keyboard_event_subscribe(void(*p_handler)(const ps2_key_state_t*));
void ps2_keyboard_process_packet();
void ps2_keyboard_init();

#endif // __DRIVERS_PS2_KEYBOARD_HPP__