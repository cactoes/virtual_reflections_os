#include "memory/vmem.hpp"
#include "memory/paging.hpp"
#include "arch/generic.hpp"
#include "multiboot.hpp"
#include "utils/bitmap.hpp"
#include "linker.hpp"

#include "interrupt_manager.hpp"
#include "process.hpp"
#include "arch/amd64/vmem.hpp"

// 2 * 64 = 128
// 128 2mb regions
// same size as defined in header
static u64 global_mapped_mmio_bitmap[2] {};

void* vmem_virtual_to_physical(void* vaddr) {
    // TODO @since 20/05/2026 -- 01:05
    // amd check ...
    return amd64_vmem_virtual_to_physical(vaddr);
}

void* vmem_map_mmio_region(void* paddr) {
    const u64 page_paddr = align_down((u64)paddr, PAGE_SIZE_LARGE);
    const u64 offset = (u64)paddr - page_paddr;

    void* vaddr = 0;

    for (size_t i = 1; i < bitmap_get_size(global_mapped_mmio_bitmap); i++) {
        if (!bitmap_get(global_mapped_mmio_bitmap, i)) {
            bitmap_set(global_mapped_mmio_bitmap, i, true);
            vaddr = (void*)(VMEM_MAPPED_MMIO_REGION + i * PAGE_SIZE_LARGE);
            break;
        }
    }

    if (vaddr == 0)
        return nullptr;

    if (!vmem_map_2mb(vaddr, (void*)page_paddr, VMEM_EXECUTE | VMEM_READWRITE | VMEM_KERNEL))
        return nullptr;

    return (void*)((u64)vaddr + offset);
}

size_t vmem_smart_alloc_pages(const void* vaddr, size_t size, u64 flags) {
    size_t allocated = 0;
    u64 current_virtual_address = (u64)vaddr;

    while (allocated < size) {
        void* target_physical_address = pmem_get_page();

        if (!target_physical_address)
            return allocated;

        // force 2mb memory alignment
        if (!is_aligned((u64)target_physical_address, PAGE_SIZE_LARGE))
            continue;

        // reserve the remaining pages to complete 2MB
        if (!pmem_try_reserve_address((void*)((u64)target_physical_address + PAGE_SIZE), (PAGE_SIZE_LARGE / PAGE_SIZE - 1)))
            return allocated;

        // map the address
        if (!vmem_map_2mb((void*)current_virtual_address, target_physical_address, flags))
            return allocated;

        // update counters
        allocated += PAGE_SIZE_LARGE;
        current_virtual_address += PAGE_SIZE_LARGE;
    }

    return allocated;
}

bool vmem_init(const void* mbstruct) {
    const u64 aligned_kernel_end_addr = align_up(LINKER_END_KERNEL_PHYS, PAGE_SIZE_LARGE);
    u64 kernel_page_count = aligned_kernel_end_addr / PAGE_SIZE;

    if (!pmem_try_reserve_address(0, kernel_page_count))
        return false;

    for (auto mm_entry = mb2_get_first_entry((multiboot2_info_t*)mbstruct); mm_entry; mm_entry = mb2_get_next_entry((multiboot2_info_t*)mbstruct, mm_entry)) {
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

bool vmem_unmap_2mb(void* vaddr) {
    // TODO @since 20/05/2026 -- 01:01
    // amd64 check bla bla bla
    return amd64_vmem_unmap_2mb(vaddr);
}

bool vmem_map_2mb(const void* vaddr, const void* paddr, u64 flags) {
    // if amd64 ->

    bool is_kernel = BIT_CHECK(flags, VMEM_BIT_USER_KERNEL);
    bool is_readwrite = BIT_CHECK(flags, VMEM_BIT_READWRITE);
    bool is_executable = BIT_CHECK(flags, VMEM_BIT_EXECUTE);

    return amd64_vmem_map_2mb(vaddr, paddr, is_kernel, is_readwrite, is_executable);

    // else ->
    // error
}

bool v2_vmem_map_2mb_remote(struct process_t* process, const void* vaddr, const void* paddr, u64 flags) {
    // if amd64 ->

    disable_interrupts();
    void* prev_page_table = get_pml4();
    set_pml4(process->page_table);

    bool result = vmem_map_2mb(vaddr, paddr, flags);

    set_pml4(prev_page_table);
    enable_interrupts();

    return result;

    // else ->
    // error
}