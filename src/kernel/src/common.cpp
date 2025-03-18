#include "common.hpp"

void* memset(void* dest, uint8_t val, size_t size) {
    if (size == 0)
        return dest;

    uint8_t* p_dest = (uint8_t*)dest;

    for (size_t i = 0; i < size; i++)
        p_dest[i] = val;

    return dest;
}

void* memzero(void* dest, size_t size) {
    return memset(dest, 0, size);
}

void* memcpy(void* dest, const void* src, size_t size) {
    if (size == 0)
        return dest;

    uint8_t* p_dest = (uint8_t*)dest;
    const uint8_t* p_src = (uint8_t*)src;

    for (size_t i = 0; i < size; i++)
        p_dest[i] = p_src[i];

    return dest;
}

bool mem_is_aligned(uint64_t addr, uint64_t align) {
    return (addr & (align - 1)) == 0;
}

uint64_t mem_align_up(uint64_t addr, uint64_t align) {
    return (addr + (align - 1)) & ~(align - 1);
}

uint64_t mem_align_down(uint64_t addr, uint64_t align) {
    return (addr) & ~(align - 1);
}