#include "libatomic.h"

// forward declares
u32 __atomic_fetch32_impl(volatile u32* ptr);
void __atomic_store32_impl(volatile u32* ptr, u32 value);
u32 __atomic_add32_impl(volatile u32* ptr, u32 value);
u32 __atomic_xchg32_impl(volatile u32* ptr, u32 value);

u32 atomic_fetch32(volatile u32* ptr) {
    return __atomic_fetch32_impl(ptr);
}

void atomic_store32(volatile u32* ptr, u32 value) {
    __atomic_store32_impl(ptr, value);
}

u32 atomic_add32(volatile u32* ptr, u32 value) {
    return __atomic_add32_impl(ptr, value);
}

u32 atomic_xchg32(volatile u32* ptr, u32 value) {
    return __atomic_xchg32_impl(ptr, value);
}