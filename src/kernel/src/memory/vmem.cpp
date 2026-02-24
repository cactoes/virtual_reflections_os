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

void* vmem_virtual_to_physical(void* pml4, void* vaddr) {
    const uint64_t pml4e =  KPAGING_GET_PE(vaddr, 39);
    const uint64_t pdpe =   KPAGING_GET_PE(vaddr, 30);
    const uint64_t pde =    KPAGING_GET_PE(vaddr, 21);

    const uint64_t* pml4_virt = GET_PML4_VIRT();
    if (!KPAGING_CHECK_ENTRY(pml4_virt, pml4e))
        return nullptr;
    
    const uint64_t* pdpt_virt = GET_PDPT_VIRT(pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt_virt, pdpe))
        return nullptr;

    const uint64_t* pdt_virt = GET_PDT_VIRT(pml4e, pdpe);
    if (!KPAGING_CHECK_ENTRY(pdt_virt, pde))
        return nullptr;

    const uint64_t page_offset = (uint64_t)vaddr & (PAGE_SIZE_LARGE - 1);
    return (void*)((pdt_virt[pde] & ~(PAGE_SIZE_LARGE - 1)) + page_offset);
}

void* vmem_map_mmio_region(void* pml4, void* paddr) {
    const uint64_t page_paddr = align_down((uint64_t)paddr, PAGE_SIZE_LARGE);
    const uint64_t offset = (uint64_t)paddr - page_paddr;

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

    if (!vmem_map_2mb(pml4, vaddr, (void*)page_paddr))
        return nullptr;

    return (void*)((uint64_t)vaddr + offset);
}


bool vmem_map_2mb(const void* pml4, const void* vaddr, const void* paddr) {
    // TODO @since 24/02/2026 -- 21:00
    // remove all the | PF_USER
    if (!is_aligned((uint64_t)vaddr, PAGE_SIZE_LARGE) ||
        !is_aligned((uint64_t)paddr, PAGE_SIZE_LARGE)) {
        return false;
    }

    const uint64_t pml4e =  KPAGING_GET_PE(vaddr, 39);
    const uint64_t pdpe =   KPAGING_GET_PE(vaddr, 30);
    const uint64_t pde =    KPAGING_GET_PE(vaddr, 21);

    uint64_t* pml4_virt = GET_PML4_VIRT();
    if (!KPAGING_CHECK_ENTRY(pml4_virt, pml4e)) {
        const void* page = pmem_get_page();
        pml4_virt[pml4e] = ((uint64_t)page & ~0xFFF) | PF_PRESENT | PF_READ_WRITE | PF_USER_SUPERVISOR;
        memzero(GET_PDPT_VIRT(pml4e), PAGE_SIZE);
    }

    uint64_t* pdpt_virt = GET_PDPT_VIRT(pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt_virt, pdpe)) {
        const void* page = pmem_get_page();
        pdpt_virt[pdpe] = ((uint64_t)page & ~0xFFF) | PF_PRESENT | PF_READ_WRITE | PF_USER_SUPERVISOR;
        memzero(GET_PDT_VIRT(pml4e, pdpe), PAGE_SIZE);
    }

    uint64_t* pdt_virt = GET_PDT_VIRT(pml4e, pdpe);
    pdt_virt[pde] = ((uint64_t)paddr & ~0x1FFFFF) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE | PF_USER_SUPERVISOR;
    flush_tlb((void*)vaddr);
    return true;
}

size_t vmem_smart_alloc_pages(const void* pml4, const void* vaddr, size_t size) {
    size_t allocated = 0;
    uint64_t current_virtual_address = (uint64_t)vaddr;

    while (allocated < size) {
        void* target_physical_address = pmem_get_page();

        if (!target_physical_address)
            return allocated;

        // force 2mb memory alignment
        if (!is_aligned((uint64_t)target_physical_address, PAGE_SIZE_LARGE))
            continue;

        // reserve the remaining pages to complete 2MB
        if (!pmem_try_reserve_address((void*)((uint64_t)target_physical_address + PAGE_SIZE), (PAGE_SIZE_LARGE / PAGE_SIZE - 1)))
            return allocated;

        // map the address
        if (!vmem_map_2mb(pml4, (void*)current_virtual_address, target_physical_address))
            return allocated;

        // update counters
        allocated += PAGE_SIZE_LARGE;
        current_virtual_address += PAGE_SIZE_LARGE;
    }

    return allocated;
}

bool vmem_init(const void* pml4, const void* mbstruct) {
    // TODO @since 24/02/2026 -- 21:00
    // remove all the | PF_USER

    // recusive map the page table
    uint64_t* pml4_entries = (uint64_t*)pml4;
    pml4_entries[511] = ((uint64_t)pml4 & ~0xFFF) | PF_PRESENT | PF_READ_WRITE | PF_USER_SUPERVISOR;

    // TODO @since 01/12/2025 -- 02:16
    // place into arch wrapper
    // reload the page tables
    asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");

    const uint64_t aligned_kernel_end_addr = align_up((uint64_t)&__lnk_end_kernel, PAGE_SIZE_LARGE);
    uint64_t kernel_page_count = aligned_kernel_end_addr / PAGE_SIZE;

    if (!pmem_try_reserve_address(0, kernel_page_count))
        return false;

    for (auto mm_entry = mb2_get_first_entry((multiboot_t*)mbstruct); mm_entry; mm_entry = mb2_get_next_entry((multiboot_t*)mbstruct, mm_entry)) {
        // reserve physical pages for reserved memory
        if (mm_entry->type != (uint32_t)memory_map_type_t::USABLE) {
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