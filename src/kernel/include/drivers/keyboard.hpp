//==========================================
/// @file       keyboard.hpp
/// @brief      generalised keyboard i/o
//==========================================

#pragma once

#ifndef __DRIVERS_KEYBOARD_HPP__
#define __DRIVERS_KEYBOARD_HPP__

enum class keyboard_selector_t {
    
};

#include "common.hpp"

char key_to_ascii(uint32_t scan_code, bool shift, bool caps);
char wait_for_key();

#endif // __DRIVERS_KEYBOARD_HPP__