#include "libmem.h"

// forward declares
void* __memzero_impl(void*, u64);
void* __memset_impl(void*, u8, u64);
void* __memcpy_impl(void*, const void*, u64);
bool __memeq_impl(const void*, const void*, u64);

void* memzero(void* dst, u64 size) {
    return __memzero_impl(dst, size);
}

void* memset(void* dst, u8 val, u64 size) {
    return __memset_impl(dst, val, size);
}

void* memcpy(void* dst, const void* src, u64 size) {
    return __memcpy_impl(dst, src, size);
}

bool memeq(const void* a, const void* b, u64 size) {
    return __memeq_impl(a, b, size);
}