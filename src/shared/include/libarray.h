//==========================================
/// @file       libarray.h
/// @brief      array based helpers like: bitmap, ...
//==========================================

#pragma once

#ifndef LIBARRAY_H
#define LIBARRAY_H

#define BITMAP_WORDS(bits)  (((bits) + 63) / 64)

#include "libtype.h"

bool bitmap_get(const u64* bitmap, u64 index);
void bitmap_set(u64* bitmap, u64 index, bool value);
void bitmap_fill(u64* bitmap, u64 words, bool value);

#endif // LIBARRAY_H