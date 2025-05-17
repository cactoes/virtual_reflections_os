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

bool bitmap_get(uint64_t* bitmap, size_t size, size_t index) {
    if (index >= bitmap_get_size(size))
        return false;

    const size_t item_index = index / 64;
    const size_t bit_index = index % 64;
    return BIT_CHECK(bitmap[item_index], bit_index);
}

void bitmap_set(uint64_t* bitmap, size_t size, size_t index, bool state) {
    if (index >= bitmap_get_size(size))
        return;

    const size_t item_index = index / 64;
    const size_t bit_index = index % 64;

    state ? BIT_SET(bitmap[item_index], bit_index)
          : BIT_CLEAR(bitmap[item_index], bit_index);
}