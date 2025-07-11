#include "memory/paging.hpp"
#include "utils/mutex.hpp"
#include "utils/bitmap.hpp"

// kernel page bitmaps
static uint64_t g_kernel_page_reserved_bitmap[KERNEL_PAGE_RESERVED_BITMAP_SIZE] {};
static uint64_t g_kernel_page_bitmap[KERNEL_PAGE_BITMAP_SIZE] {};

// no init required here since mutex init sets locked to false
static mutex_t g_kprb_mutex { .locked = 0 };
static mutex_t g_kpb_mutex { .locked = 0 };

void* pmem_get_page_reserved() {
    const mutex_lock_guard guard(&g_kprb_mutex);

    // slow ahh
    for (size_t i = 1; i < bitmap_get_size(g_kernel_page_reserved_bitmap); i++) {
        if (!bitmap_get(g_kernel_page_reserved_bitmap, i)) {
            bitmap_set(g_kernel_page_reserved_bitmap, i, true);
            return (void*)(i * PAGE_SIZE);
        }
    }

    return nullptr;
}

void* pmem_get_page() {
    const mutex_lock_guard guard(&g_kpb_mutex);

    // slow ahh
    for (size_t i = 1; i < bitmap_get_size(g_kernel_page_bitmap); i++) {
        if (!bitmap_get(g_kernel_page_bitmap, i)) {
            bitmap_set(g_kernel_page_bitmap, i, true);
            return (void*)((i + bitmap_get_size(g_kernel_page_reserved_bitmap)) * PAGE_SIZE);
        }
    }

    return nullptr;
}

bool pmem_reserve_at_adress(uint64_t address, size_t count) {
    if (!is_aligned(address, PAGE_SIZE))
        return false;

    constexpr uint64_t end_critical_memory_space = bitmap_get_size(g_kernel_page_reserved_bitmap) * PAGE_SIZE;
    constexpr uint64_t end_memory_space = end_critical_memory_space + bitmap_get_size(g_kernel_page_bitmap) * PAGE_SIZE;

    size_t pages_reserved = 0;

    while (pages_reserved < count) {
        if (address < end_critical_memory_space) {
            size_t index = address / PAGE_SIZE;
            if (index >= bitmap_get_size(g_kernel_page_reserved_bitmap))
                return false;

            bitmap_set(g_kernel_page_reserved_bitmap, index, true);
        } else if (address < end_memory_space) {
            size_t index = (address - end_critical_memory_space) / PAGE_SIZE;
            if (index >= bitmap_get_size(g_kernel_page_bitmap))
                return false;

            bitmap_set(g_kernel_page_bitmap, index, true);
        } else {
            return false;
        }

        pages_reserved++;
        address += PAGE_SIZE;
    }

    return true;
}

bool pmem_is_in_memory_range(void* p_addr) {
    return (uint64_t)p_addr <= (bitmap_get_size(g_kernel_page_bitmap) + bitmap_get_size(g_kernel_page_reserved_bitmap)) * PAGE_SIZE;
}