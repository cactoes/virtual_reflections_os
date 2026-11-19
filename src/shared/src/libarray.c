#include "libarray.h"

bool bitmap_get(const u64* bitmap, u64 index) {
    return (bitmap[index / 64] >> (index % 64)) & 1;
}

void bitmap_set(u64* bitmap, u64 index, bool value) {
    if (value)
        bitmap[index / 64] |= (1ull << (index % 64));
    else
        bitmap[index / 64] &= ~(1ull << (index % 64));
}

void bitmap_fill(u64* bitmap, u64 words, bool value) {
    for (u64 i = 0; i < words; i++)
        bitmap[i] = value ? ~0ull : 0ull;
}