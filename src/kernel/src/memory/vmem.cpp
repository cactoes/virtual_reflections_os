#include "memory/vmem.hpp"
#include "memory/paging.hpp"
#include "arch/generic.hpp"
#include "multiboot.hpp"
#include "utils/bitmap.hpp"

// 2 * 64 = 128
// 128 2mb regions
static uint64_t global_mapped_mmio_bitmap[2] {};

/// @brief variable placed at the end of the kernel
// NOLINTNEXTLINE
extern "C" uint64_t __lnk_end_kernel;

void vmem_identity_map(void* p_pml4) {
    if (!KPAGING_CHECK_ENTRY(p_pml4, 0)) {
        auto page = pmem_get_page_reserved();
        ((uint64_t*)p_pml4)[0] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdpt = KPAGING_GET_ENTRY(p_pml4, 0);
    if (!KPAGING_CHECK_ENTRY(pdpt, 0)) {
        auto page = pmem_get_page_reserved();
        pdpt[0] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    // identity map the first 1 GB
    auto pdt = KPAGING_GET_ENTRY(pdpt, 0);
    for (uint64_t pde = 0; pde < 512; pde++) {
        uint64_t physical_addr = (uint64_t)pde * PAGE_SIZE_LARGE;
        pdt[pde] = (physical_addr & ~0x1FFFFF) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE;
        flush_tlb((void*)physical_addr);
    }
}

bool vmem_map_2kb_page(void* p_pml4, void* p_virtual_addr, void* p_physical_addr) {
    if (!is_aligned((uint64_t)p_virtual_addr, PAGE_SIZE) ||
        !is_aligned((uint64_t)p_physical_addr, PAGE_SIZE)) {
        return false;
    }

    const uint64_t pml4e =    KPAGING_GET_PE(p_virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(p_virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(p_virtual_addr, 21);
    const uint64_t pte =      KPAGING_GET_PE(p_virtual_addr, 12);

    if (!KPAGING_CHECK_ENTRY(p_pml4, pml4e)) {
        auto page = pmem_get_page_reserved();
        ((uint64_t*)p_pml4)[pml4e] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdpt = KPAGING_GET_ENTRY(p_pml4, pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe)) {
        auto page = pmem_get_page_reserved();
        pdpt[pdpe] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdt = KPAGING_GET_ENTRY(pdpt, pdpe);
    if (!KPAGING_CHECK_ENTRY(pdt, pde)) {
        auto page = pmem_get_page_reserved();
        pdt[pde] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto ptt = KPAGING_GET_ENTRY(pdt, pde);
    ptt[pte] = ((uint64_t)p_physical_addr & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;
    flush_tlb(p_virtual_addr);

    return true;
}

bool vmem_map_2mb_page(void* p_pml4, void* p_virtual_addr, void* p_physical_addr) {
    if (!is_aligned((uint64_t)p_virtual_addr, PAGE_SIZE_LARGE) ||
        !is_aligned((uint64_t)p_physical_addr, PAGE_SIZE_LARGE)) {
        return false;
    }

    const uint64_t pml4e =    KPAGING_GET_PE(p_virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(p_virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(p_virtual_addr, 21);

    if (!KPAGING_CHECK_ENTRY(p_pml4, pml4e)) {
        auto page = pmem_get_page_reserved();
        ((uint64_t*)p_pml4)[pml4e] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    uint64_t* pdpt = KPAGING_GET_ENTRY(p_pml4, pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe)) {
        auto page = pmem_get_page_reserved();
        pdpt[pdpe] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    uint64_t* pdt = KPAGING_GET_ENTRY(pdpt, pdpe);
    pdt[pde] = ((uint64_t)p_physical_addr & ~0x1FFFFF) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE;
    flush_tlb(p_virtual_addr);

    return true;
}

size_t vmem_smart_alloc_pages(void* p_pml4, void* p_virtual_addr, size_t size) {
    size_t allocated = 0;
    uint64_t current_virtual_address = (uint64_t)p_virtual_addr;

    while (allocated < size) {
        void* target_physical_address = pmem_get_page();

        if (!target_physical_address)
            return allocated;

        // force 2mb memory alignment
        if (!is_aligned((uint64_t)target_physical_address, PAGE_SIZE_LARGE))
            continue;

        // reserve the remaining pages to complete 2MB
        if (!pmem_reserve_at_adress((uint64_t)target_physical_address + PAGE_SIZE, (PAGE_SIZE_LARGE / PAGE_SIZE - 1)))
            return allocated;

        // map the address
        if (!vmem_map_2mb_page(p_pml4, (void*)current_virtual_address, target_physical_address))
            return allocated;

        // update counters
        allocated += PAGE_SIZE_LARGE;
        current_virtual_address += PAGE_SIZE_LARGE;
    }

    return allocated;
}

void* vmem_virtual_to_physical(void* p_pml4, void* p_virtual_addr) {
    const uint64_t pml4e =    KPAGING_GET_PE(p_virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(p_virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(p_virtual_addr, 21);

    if (!KPAGING_CHECK_ENTRY(p_pml4, pml4e))
        return 0;
    
    uint64_t* pdpt = KPAGING_GET_ENTRY(p_pml4, pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe))
        return 0;

    uint64_t* pdt = KPAGING_GET_ENTRY(pdpt, pdpe);
    if (!KPAGING_CHECK_ENTRY(pdt, pde))
        return 0;

    const uint64_t page_offset = (uint64_t)p_virtual_addr & (PAGE_SIZE_LARGE - 1);
    return (void*)((pdt[pde] & ~(PAGE_SIZE_LARGE - 1)) + page_offset);
}

bool vmem_init(void* p_multiboot_struct, void* p_pml4) {
    // zero page (?)
    memzero(0, PAGE_SIZE);

    const uint64_t aligned_kernel_end_addr = align_up((uint64_t)&__lnk_end_kernel, PAGE_SIZE_LARGE);
    const uint64_t kernel_page_count = aligned_kernel_end_addr / PAGE_SIZE;
    if (!pmem_reserve_at_adress(0, kernel_page_count))
        return false;

    for (auto mm_entry = mb2_get_first_entry((multiboot_t*)p_multiboot_struct); mm_entry; mm_entry = mb2_get_next_entry((multiboot_t*)p_multiboot_struct, mm_entry)) {
        // reserve physical pages for reserved memory
        if (mm_entry->type != (uint32_t)memory_map_type_t::USABLE) {
            if (!pmem_is_in_memory_range((void*)(mm_entry->addr + mm_entry->len)))
                continue;

            // reserve as much as possible
            for (size_t i = 0; i < mm_entry->len; i += PAGE_SIZE) {
                if (!pmem_reserve_at_adress(align_down(mm_entry->addr, PAGE_SIZE) + i, 1))
                    return false;
            }
        }
    }

    // // first 1G is identity mapped
    // vmem_identity_map((uint64_t*)pml4);
    // __set_pml4(pml4);

    return true;
}

void* vmem_map_mmio_region(void* pml4, void* physical_address) {
    uint64_t page_addr_physical = align_down((uint64_t)physical_address, PAGE_SIZE_LARGE);
    uint64_t addr_offset = (uint64_t)physical_address - page_addr_physical;

    void* virtual_address = 0;

    for (size_t i = 1; i < bitmap_get_size(global_mapped_mmio_bitmap); i++) {
        if (!bitmap_get(global_mapped_mmio_bitmap, i)) {
            bitmap_set(global_mapped_mmio_bitmap, i, true);
            virtual_address = (void*)(VMEM_MAPPED_MMIO_REGION + i * PAGE_SIZE_LARGE);
            break;
        }
    }

    if (virtual_address == 0)
        return nullptr;

    if (!vmem_map_2mb_page(pml4, virtual_address, (void*)page_addr_physical))
        return nullptr;

    return (void*)((uint64_t)virtual_address + addr_offset);
}