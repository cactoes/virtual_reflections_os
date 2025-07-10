#include "common.hpp"

void* memset(void* p_dest, uint8_t val, size_t size) {
    if (size == 0)
        return p_dest;

    uint8_t* p_dest_tmp = (uint8_t*)p_dest;

    for (size_t i = 0; i < size; i++)
        p_dest_tmp[i] = val;

    return p_dest;
}

void* memzero(void* p_dest, size_t size) {
    return memset(p_dest, 0, size);
}

void* memcpy(void* p_dest, const void* p_src, size_t size) {
    if (size == 0)
        return p_dest;

    uint8_t* p_dest_tmp = (uint8_t*)p_dest;
    const uint8_t* p_src_tmp = (uint8_t*)p_src;

    for (size_t i = 0; i < size; i++)
        p_dest_tmp[i] = p_src_tmp[i];

    return p_dest;
}

bool memeq(const void* p_a1, const void* p_a2, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (((uint8_t*)p_a1)[i] != ((uint8_t*)p_a2)[i])
            return false;
    }

    return true;
}

bool is_aligned(uint64_t addr, uint64_t align) {
    return (addr & (align - 1)) == 0;
}

uint64_t align_up(uint64_t addr, uint64_t align) {
    return (addr + (align - 1)) & ~(align - 1);
}

uint64_t align_down(uint64_t addr, uint64_t align) {
    return (addr) & ~(align - 1);
}