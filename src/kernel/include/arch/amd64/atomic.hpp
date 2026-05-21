//==========================================
/// @file       atomic.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AMD64_ATOMIC_HPP__
#define __AMD64_ATOMIC_HPP__

static inline
int amd64_atomic_exchange(volatile int* ptr, int value) {
    int old;
    asm volatile (
        "lock xchg %0, %1"
        : "=r"(old), "+m"(*ptr)
        : "0"(value)
        : "memory"
    );
    return old;
}

static inline
int amd64_atomic_fetch_add(volatile int* ptr, int value) {
    asm volatile(
        "lock xadd %0, %1"
        : "+r"(value), "+m"(*ptr)
        :
        : "memory"
    );
    return value;
}

static inline
int amd64_atomic_fetch_sub(volatile int* ptr, int value) {
    value = -value;
    asm volatile(
        "lock xadd %0, %1"
        : "+r"(value), "+m"(*ptr)
        :
        : "memory"
    );
    return value;
}

#endif // __AMD64_ATOMIC_HPP__