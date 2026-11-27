//==========================================
/// @file       kterminal.h
/// @brief      output only terminal interface
//==========================================

#pragma once

#ifndef KTERMINAL_H
#define KTERMINAL_H

#define KTERM_CHAR_WIDTH  8u
#define KTERM_CHAR_HEIGHT 16u

#define KTERM_ANSI_TEXT 0u
#define KTERM_ANSI_ESC  1u
#define KTERM_ANSI_CSI  2u

#define KTERM_VIEWPORT_WIDTH_DIVISOR 3u

#define KTERM_PRINTF_BUFFER_SIZE 1024u

#include "libtype.h"

typedef struct __kterminal {
    u64 columns;
    u64 rows;
    u64 column;
    u64 row;

    u64 viewport_x;
    u64 viewport_y;
    u64 viewport_width;
    u64 viewport_height;

    u32 foreground;
    u32 background;
    u32 default_foreground;
    u32 default_background;

    bool initialized;

    u8 ansi_state;
    u8 ansi_param_count;
    u32 ansi_params[8];
    u32 ansi_value;
    bool ansi_have_digit;
} kterminal_t;

bool kterm_init(kterminal_t* term);
void kterm_bind(kterminal_t* term);
bool kterm_printf(const char* fmt, ...);

#endif // KTERMINAL_H