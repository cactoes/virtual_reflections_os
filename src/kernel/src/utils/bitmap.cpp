#include "utils/bitmap.hpp"

bool bitmap_get(u64* p_bitmap, size_t size, size_t index) {
    if (index >= bitmap_get_size(size))
        return false;

    const size_t item_index = index / 64;
    const size_t bit_index = index % 64;
    return BIT_CHECK(p_bitmap[item_index], bit_index);
}

void bitmap_set(u64* p_bitmap, size_t size, size_t index, bool state) {
    if (index >= bitmap_get_size(size))
        return;

    const size_t item_index = index / 64;
    const size_t bit_index = index % 64;

    state ? BIT_SET(p_bitmap[item_index], bit_index)
          : BIT_CLEAR(p_bitmap[item_index], bit_index);
}
