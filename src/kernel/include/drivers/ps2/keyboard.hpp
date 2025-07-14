//==========================================
/// @file       keyboard.hpp
/// @brief      ps2 keyboard driver
//==========================================

#pragma once

#ifndef __DRIVERS_PS2_KEYBOARD_HPP__
#define __DRIVERS_PS2_KEYBOARD_HPP__

#include "common.hpp"

void ps2_keyboard_handle_interrupt();
uint64_t ps2_keyboard_get_last_scancode();
void ps2_keyboard_clear_last_scancode();

#endif // __DRIVERS_PS2_KEYBOARD_HPP__