//==========================================
/// @file       ansi.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __ANSI_HPP__
#define __ANSI_HPP__

#define ANSI_ESCAPE_CHAR '\033'

#include "common.hpp"
#include "drivers/graphics/graphics_driver.hpp"

enum class ansi_color_t : u8 {
    BLACK           = 0,
    RED             = 1,
    GREEN           = 2,
    BROWN           = 3,
    BLUE            = 4,
    MAGENTA         = 5,
    CYAN            = 6,
    LIGHT_GRAY      = 7,
    DARK_GRAY       = 8,
    LIGHT_BLUE      = 9,
    LIGHT_GREEN     = 10,
    LIGHT_CYAN      = 11,
    LIGHT_RED       = 12,
    LIGHT_MAGENTA   = 13,
    YELLOW          = 14,
    WHITE           = 15,
};

constexpr color_t ansi_to_rgb(ansi_color_t color) {
    switch (color) {
        case ansi_color_t::BLACK:           return {0x00, 0x00, 0x00};
        case ansi_color_t::BLUE:            return {0x00, 0x00, 0xAA};
        case ansi_color_t::GREEN:           return {0x00, 0xAA, 0x00};
        case ansi_color_t::CYAN:            return {0x00, 0xAA, 0xAA};
        case ansi_color_t::RED:             return {0xAA, 0x00, 0x00};
        case ansi_color_t::MAGENTA:         return {0xAA, 0x00, 0xAA};
        case ansi_color_t::BROWN:           return {0xAA, 0x55, 0x00};
        case ansi_color_t::LIGHT_GRAY:      return {0xAA, 0xAA, 0xAA};
        case ansi_color_t::DARK_GRAY:       return {0x55, 0x55, 0x55};
        case ansi_color_t::LIGHT_BLUE:      return {0x55, 0x55, 0xFF};
        case ansi_color_t::LIGHT_GREEN:     return {0x55, 0xFF, 0x55};
        case ansi_color_t::LIGHT_CYAN:      return {0x55, 0xFF, 0xFF};
        case ansi_color_t::LIGHT_RED:       return {0xFF, 0x55, 0x55};
        case ansi_color_t::LIGHT_MAGENTA:   return {0xFF, 0x55, 0xFF};
        case ansi_color_t::YELLOW:          return {0xFF, 0xFF, 0x55};
        case ansi_color_t::WHITE:           return {0xFF, 0xFF, 0xFF};
    }

    return {0x00, 0x00, 0x00};
}

#endif // __ANSI_HPP__