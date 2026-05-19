#include "arch/amd64/vmem.hpp"
#include "memory/paging.hpp"

bool amd64_vmem_map_2mb(const void* vaddr, const void* paddr, bool is_kernel, bool is_readwrite, bool is_executeable) {
    // TODO @since 20/05/2026 -- 00:26
    // readwrite, executable

    if (!is_aligned((u64)vaddr, PAGE_SIZE_LARGE) ||
        !is_aligned((u64)paddr, PAGE_SIZE_LARGE)) {
        return false;
    }

    const u64 pml4e =  KPAGING_GET_PE(vaddr, 39);
    const u64 pdpe =   KPAGING_GET_PE(vaddr, 30);
    const u64 pde =    KPAGING_GET_PE(vaddr, 21);

    u64* pml4_virt = GET_PML4_VIRT();
    if (!KPAGING_CHECK_ENTRY(pml4_virt, pml4e)) {
        const void* page = pmem_get_page();
        pml4_virt[pml4e] = ((u64)page & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;
        memzero(GET_PDPT_VIRT(pml4e), PAGE_SIZE);
    }

    u64* pdpt_virt = GET_PDPT_VIRT(pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt_virt, pdpe)) {
        const void* page = pmem_get_page();
        pdpt_virt[pdpe] = ((u64)page & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;
        memzero(GET_PDT_VIRT(pml4e, pdpe), PAGE_SIZE);
    }

    // FIXME @since 05/03/2026 -- 12:21
    // MAKE SURE THE PARENT PAGE TABLES DONT CONTAIN KERNEL PAGE ENTRIES
    if (!is_kernel) {
        pdpt_virt[pdpe] |= PF_USER_SUPERVISOR;
        pml4_virt[pml4e] |= PF_USER_SUPERVISOR;
    }

    const u64 basic_page_flags = !is_kernel
        ? PF_PRESENT | PF_READ_WRITE | PF_USER_SUPERVISOR
        : PF_PRESENT | PF_READ_WRITE;

    u64* pdt_virt = GET_PDT_VIRT(pml4e, pdpe);
    pdt_virt[pde] = ((u64)paddr & ~0x1FFFFF) | PF_PAGE_SIZE | basic_page_flags;
    amd64_flush_tlb((void*)vaddr);
    return true;
}

bool amd64_vmem_unmap_2mb(void* vaddr) {
    // BUG @since 26/02/2026 -- 11:55
    // empty enties get removed but are still alocated by the page allocator

    if (!is_aligned((u64)vaddr, PAGE_SIZE_LARGE))
        return false;

    const u64 pml4e =  KPAGING_GET_PE(vaddr, 39);
    const u64 pdpe =   KPAGING_GET_PE(vaddr, 30);
    const u64 pde =    KPAGING_GET_PE(vaddr, 21);

    u64* pml4_virt = GET_PML4_VIRT();
    if (!KPAGING_CHECK_ENTRY(pml4_virt, pml4e))
        return false;

    u64* pdpt_virt = GET_PDPT_VIRT(pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt_virt, pdpe))
        return false;

    u64* pdt_virt = GET_PDT_VIRT(pml4e, pdpe);
    pdt_virt[pde] = 0;
    amd64_flush_tlb((void*)vaddr);
    return true;
}

void* amd64_vmem_virtual_to_physical(void* vaddr) {
    const u64 pml4e =  KPAGING_GET_PE(vaddr, 39);
    const u64 pdpe =   KPAGING_GET_PE(vaddr, 30);
    const u64 pde =    KPAGING_GET_PE(vaddr, 21);

    const u64* pml4_virt = GET_PML4_VIRT();
    if (!KPAGING_CHECK_ENTRY(pml4_virt, pml4e))
        return nullptr;
    
    const u64* pdpt_virt = GET_PDPT_VIRT(pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt_virt, pdpe))
        return nullptr;

    const u64* pdt_virt = GET_PDT_VIRT(pml4e, pdpe);
    if (!KPAGING_CHECK_ENTRY(pdt_virt, pde))
        return nullptr;

    const u64 page_offset = (u64)vaddr & (PAGE_SIZE_LARGE - 1);
    return (void*)((pdt_virt[pde] & ~(PAGE_SIZE_LARGE - 1)) + page_offset);
}