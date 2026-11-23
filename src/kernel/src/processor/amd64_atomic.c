#include "libtype.h"

u32 __atomic_fetch32_impl(volatile u32* ptr) {
    u32 value;
    __asm__ volatile(
        "movl %1, %0"
        : "=r" (value)
        : "m" (*ptr)
        : "memory"
    );
    return value;
}

void __atomic_store32_impl(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        "movl %1, %0"
        : "=m" (*ptr)
        : "r" (value)
        : "memory"
    );
}

u32 __atomic_add32_impl(volatile u32* ptr, u32 value) {
    __asm__ volatile(
        "lock xaddl %0, %1"
        : "+r"(value), "+m"(*ptr)
        :
        : "memory", "cc"
    );
    return value;
}