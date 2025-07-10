//==========================================
/// @file       bitmap.hpp
/// @brief      bitmap impl
//==========================================

#pragma once

#ifndef __UTILS_BITMAP_HPP__
#define __UTILS_BITMAP_HPP__

#include "common.hpp"

constexpr uint64_t bitmap_get_size(size_t size) {
    // NOLINTNEXTLINE
    return size * (sizeof(uint64_t) * 8);
}

template <size_t size>
constexpr uint64_t bitmap_get_size(uint64_t (&bitmap)[size]) {
    return bitmap_get_size(size);
}

bool bitmap_get(uint64_t* p_bitmap, size_t size, size_t index);

template <size_t size>
bool bitmap_get(uint64_t (&bitmap)[size], size_t index) {
    return bitmap_get(bitmap, size, index);
}

void bitmap_set(uint64_t* p_bitmap, size_t size, size_t index, bool state);

template <size_t size>
void bitmap_set(uint64_t (&bitmap)[size], size_t index, bool state) {
    return bitmap_set(bitmap, size, index, state);
}

#endif // __UTILS_BITMAP_HPP__