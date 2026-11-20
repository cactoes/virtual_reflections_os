//==========================================
/// @file       generic.h
/// @brief      
//==========================================

#pragma once

#ifndef GENERIC_H
#define GENERIC_H

#define KERNEL_HEAP_BASE        0xfffff81000000000ull
#define KERNEL_HEAP_END         0xfffff83000000000ull
#define KERNEL_MMIO_BASE        0xfffff84000000000ull
#define KERNEL_MMIO_END         0xfffff86000000000ull
#define KERNEL_STACK_BASE       0xfffff86000000000ull
#define KERNEL_STACK_END        0xfffff87000000000ull

#define USER_SPACE_BASE         0x0000000000000000ull
#define USER_SPACE_END          0x00007fffffffffffull

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