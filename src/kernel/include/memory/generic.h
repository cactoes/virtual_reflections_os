//==========================================
/// @file       generic.h
/// @brief      
//==========================================

#pragma once

#ifndef GENERIC_H
#define GENERIC_H

#include "libtype.h"

void* kalloc(u64 size);
void* kcalloc(u64 size, u8 value);
void kfree(void* ptr);

static inline
bool is_aligned(u64 addr, u64 align) {
    return (addr & (align - 1)) == 0;
}

static inline
u64 align_up(u64 addr, u64 align) {
    return (addr + (align - 1)) & ~(align - 1);
}

static inline
u64 align_down(u64 addr, u64 align) {
    return (addr) & ~(align - 1);
}

#endif // GENERIC_H