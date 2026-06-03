#include "memory/paging.hpp"
#include "utils/mutex.hpp"
#include "utils/bitmap.hpp"
#include "system_info.hpp"
#include "linker.hpp"
#include "multiboot.hpp"

// TODO @since 26/05/2026 -- 20:10
// add some state?

static u64 global_page_bitmap[PAGING_BITMAP_SIZE] {};
static mutex_t global_pmem_mutex { .locked = 0 };
pmem_info_t global_pmem_info {};

void* pmem_get_page() {
    const mutex_lock_guard guard(&global_pmem_mutex);

    const size_t system_memory_kb = global_pmem_info.memory_size * 0.001;
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

    if (!is_aligned((u64)paddr, PAGE_SIZE))
        return false;

    constexpr u64 end_of_memory = bitmap_get_size(global_page_bitmap) * PAGE_SIZE;

    for (size_t pages_reserved = 0; pages_reserved < count; pages_reserved++) {
        const u64 address_current = (u64)paddr + (pages_reserved * PAGE_SIZE);

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
    return (u64)address <= bitmap_get_size(global_page_bitmap) * PAGE_SIZE;
}

bool pmem_init(void* multiboot_struct) {
    for (auto mm_entry = mb2_get_first_entry((multiboot2_info_t*)multiboot_struct); mm_entry; mm_entry = mb2_get_next_entry((multiboot2_info_t*)multiboot_struct, mm_entry))
        if (mm_entry->type == (u32)memory_map_type_t::USABLE)
            global_pmem_info.memory_size += mm_entry->len;

    const u64 aligned_kernel_end_addr = align_up(LINKER_END_KERNEL_PHYS, PAGE_SIZE_LARGE);
    u64 kernel_page_count = aligned_kernel_end_addr / PAGE_SIZE;

    if (!pmem_try_reserve_address(0, kernel_page_count))
        return false;

    for (auto mm_entry = mb2_get_first_entry((multiboot2_info_t*)multiboot_struct); mm_entry; mm_entry = mb2_get_next_entry((multiboot2_info_t*)multiboot_struct, mm_entry)) {
        // reserve physical pages for reserved memory
        if (mm_entry->type != (u32)memory_map_type_t::USABLE) {
            if (!pmem_is_in_memory_range((void*)(mm_entry->addr + mm_entry->len)))
                continue;

            // reserve as much as possible
            for (size_t i = 0; i < mm_entry->len; i += PAGE_SIZE) {
                if (!pmem_try_reserve_address((void*)(align_down(mm_entry->addr, PAGE_SIZE) + i), 1))
                    return false;
            }
        }
    }

    return true;
}