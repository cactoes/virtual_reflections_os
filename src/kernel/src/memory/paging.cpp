#include "memory/paging.hpp"
#include "utils/mutex.hpp"
#include "utils/bitmap.hpp"
#include "system_info.hpp"

static uint64_t global_page_bitmap[PAGING_BITMAP_SIZE] {};
static mutex_t global_pmem_mutex { .locked = 0 };

void* pmem_get_page() {
    const mutex_lock_guard guard(&global_pmem_mutex);

    const size_t system_memory_kb = get_global_system_info_manager()->memory_size * 0.001;
    const size_t max_allocatable_size_kb = bitmap_get_size(global_page_bitmap);

    for (size_t i = 1; i < max_allocatable_size_kb && i < system_memory_kb; i++) {
        if (!bitmap_get(global_page_bitmap, i)) {
            bitmap_set(global_page_bitmap, i, true);
            return (void*)(i * PAGE_SIZE);
        }
    }

    return nullptr;
}

bool pmem_try_reserve_address(const void* paddr, size_t count) {
    // THIS FUNCTION LEAKS PAGES

    if (!is_aligned((uint64_t)paddr, PAGE_SIZE))
        return false;

    constexpr uint64_t end_of_memory = bitmap_get_size(global_page_bitmap) * PAGE_SIZE;

    for (size_t pages_reserved = 0; pages_reserved < count; pages_reserved++) {
        const uint64_t address_current = (uint64_t)paddr + (pages_reserved * PAGE_SIZE);

        if (address_current >= end_of_memory)
            // here
            return false;

        const size_t index = address_current / PAGE_SIZE;
        if (index >= bitmap_get_size(global_page_bitmap))
            // & here
            return false;

        bitmap_set(global_page_bitmap, index, true);
    }

    return true;
}

bool pmem_is_in_memory_range(const void* address) {
    return (uint64_t)address <= bitmap_get_size(global_page_bitmap) * PAGE_SIZE;
}