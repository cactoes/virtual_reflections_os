//==========================================
/// @file       libstring.h
/// @brief      
//==========================================

#pragma once

#ifndef LIBSTRING_H
#define LIBSTRING_H

#include <stdarg.h>
#include "libtype.h"

typedef struct __fmt_out {
    char* buf;
    u64 size;
    u64 pos;
} fmt_out_t;

u64 strlen(const char* str);

u64 vsprintf(char* buf, u64 size, const char* fmt, va_list ap);
u64 sprintf(char* buf, u64 size, const char* fmt, ...);

#endif // LIBSTRING_H