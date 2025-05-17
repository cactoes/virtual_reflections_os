//==========================================
/// @file       common.hpp
/// @brief      general items that are used
///             in the entire kernel
//==========================================

#pragma once

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#define PAGE_SIZE           0x1000
#define PAGE_SIZE_LARGE     0x200000
#define PAGE_SIZE_HUGE      0x40000000

#define ABS(a)              (((a) < 0) ? -(a) : (a))
#define MIN(a, b)           ((a) > (b) ? (b) : (a))
#define MAX(a, b)           ((a) < (b) ? (b) : (a))
#define CLAMP(x, a, b)      (MIN(MAX((x), (a)), (b)))

#define KB(x)               ((x) * 1024UL)
#define MB(x)               (KB(x) * 1024UL)
#define GB(x)               (MB(x) * 1024UL)

#define UNUSED(x)           (void)(x)

#define NOINLINE            __attribute__((noinline))
#define ALWAYS_INLINE       __attribute__((always_inline))
#define PACKED              __attribute__((packed))
#define NAKED               __attribute__((naked))
#define NODISCARD           [[nodiscard]]
#define NORETURN            [[noreturn]]
#define UNUSED_PARAM        [[maybe_unused]]

#define BIT(n)              (1UL << (n))
#define BIT_SET(x, n)       ((x) |= BIT(n))
#define BIT_CLEAR(x, n)     ((x) &= ~BIT(n))
#define BIT_TOGGLE(x, n)    ((x) ^= BIT(n))
#define BIT_CHECK(x, n)     (((x) >> (n)) & 1U)

typedef long unsigned int size_t;

typedef unsigned long long uint64_t;
typedef          long long int64_t;

typedef unsigned int uint32_t;
typedef          int int32_t;

typedef unsigned short uint16_t;
typedef          short int16_t;

typedef unsigned char uint8_t;
typedef   signed char int8_t;

void* memset(void* dest, uint8_t val, size_t size);
void* memzero(void* dest, size_t size);
void* memcpy(void* dest, const void* src, size_t size);

bool mem_is_aligned(uint64_t addr, uint64_t align);
uint64_t mem_align_up(uint64_t addr, uint64_t align);
uint64_t mem_align_down(uint64_t addr, uint64_t align);

constexpr uint64_t bitmap_get_size(size_t size) {
    return size * (sizeof(uint64_t) * 8);
}

template <size_t size>
constexpr uint64_t bitmap_get_size(uint64_t (&bitmap)[size]) {
    return bitmap_get_size(size);
}

bool bitmap_get(uint64_t* bitmap, size_t size, size_t index);

template <size_t size>
bool bitmap_get(uint64_t (&bitmap)[size], size_t index) {
    return bitmap_get(bitmap, size, index);
}

void bitmap_set(uint64_t* bitmap, size_t size, size_t index, bool state);

template <size_t size>
void bitmap_set(uint64_t (&bitmap)[size], size_t index, bool state) {
    return bitmap_set(bitmap, size, index, state);
}

constexpr uint64_t hash_fnv1a_64(const char* str, uint64_t hash = 14695981039346656037ULL) {
    return (*str == '\0') ? hash :
        hash_fnv1a_64(str + 1, (hash ^ static_cast<uint64_t>(*str)) * 1099511628211ULL);
}

constexpr uint64_t hash_string_64(const char* str, uint64_t hash = 0ULL) {
    return (*str == '\0') ? hash :
        hash_string_64(str + 1, (hash << 1) + static_cast<uint64_t>(*str));
}

#endif // __COMMON_HPP__