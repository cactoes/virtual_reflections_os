#include "processor/amd64_vmm.h"
#include "memory/vmm.h"
#include "memory/generic.h"
#include "memory/pmm.h"
#include "libmem.h"

static
void flush_tlb(void* vaddr) {
    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"(vaddr)
        : "memory"
    );
}

static
u64* vmm_write_next_table(u64* table, u64 index, u64* next_virt, bool user) {
    const u64 entry = table[index];

    if (entry & AMD64_PTE_PRESENT) {
        if (entry & AMD64_PTE_HUGE)
            return nullptr;
        if (user)
            table[index] |= AMD64_PTE_USER;
        return next_virt;
    }

    void* page = pmm_alloc_pages(1);
    if (!page)
        return nullptr;

    table[index] = ((u64)page & AMD64_PTE_ADDR_MASK_4K)
                    | AMD64_PTE_PRESENT | AMD64_PTE_WRITE
                    | (user ? AMD64_PTE_USER : 0);
    memzero(next_virt, VMEM_PAGE_SIZE_SMALL);
    return next_virt;
}

static
u64 amd64_page_addr_mask(u64 page_size) {
    switch (page_size) {
        case VMEM_PAGE_SIZE_HUGE: return AMD64_PTE_ADDR_MASK_1G;
        case VMEM_PAGE_SIZE_LARGE: return AMD64_PTE_ADDR_MASK_2M;
        default: return AMD64_PTE_ADDR_MASK_4K;
    }
}

static
bool vmm_write_mapping(u64* table, u64 index, const void* vaddr, const void* paddr, u64 flags, u64 page_size) {
    u64 mask = amd64_page_addr_mask(page_size);
    table[index] = ((u64)paddr & mask) | flags;
    flush_tlb((void*)vaddr);
    return true;
}

static
u64* amd64_vmm_get_table(u64* table, u64 index, u64* next_virt) {
    const u64 entry = table[index];
    if (!(entry & AMD64_PTE_PRESENT) || (entry & AMD64_PTE_HUGE))
        return nullptr;

    return next_virt;
}

static
bool vmm_clear_mapping(u64* table, u64 index, const void* vaddr, bool is_huge, u64 page_size, u64* old_paddr) {
    const u64 entry = table[index];
    if (!(entry & AMD64_PTE_PRESENT))
        return false;

    if (is_huge && !(entry & AMD64_PTE_HUGE))
        return false;

    if (old_paddr)
        *old_paddr = entry & amd64_page_addr_mask(page_size);

    table[index] = 0;
    flush_tlb((void*)vaddr);
    return true;
}

u64 amd64_vmm_flags_to_pte(u64 flags, u64 page_size, bool nx_enabled) {
    u64 entry = AMD64_PTE_PRESENT;
 
    if (flags & VMEM_WRITE)
        entry |= AMD64_PTE_WRITE;
    if (flags & VMEM_USER)
        entry |= AMD64_PTE_USER;
    if (!(flags & VMEM_EXEC) && nx_enabled)
        entry |= AMD64_PTE_NX;
    if ((flags & VMEM_GLOBAL) && !(flags & VMEM_USER))
        entry |= AMD64_PTE_GLOBAL;
 
    const bool is_4k = (page_size == VMEM_PAGE_SIZE_SMALL);
    const u64 pat_bit = is_4k ? AMD64_PTE_PAT_4K : AMD64_PTE_PAT_LARGE;
    switch (flags & VMEM_CACHE_MASK) {
        case VMEM_CACHE_WB: break;
        case VMEM_CACHE_WT: entry |= AMD64_PTE_PWT; break;
        case VMEM_CACHE_UC: entry |= AMD64_PTE_PCD | AMD64_PTE_PWT; break;
        case VMEM_CACHE_WC: entry |= pat_bit; break;
    }
 
    if (!is_4k)
        entry |= AMD64_PTE_HUGE;
 
    return entry;
}

bool amd64_vmm_page_already_mapped(const void* vaddr, u64 page_size) {
    if (page_size != VMEM_PAGE_SIZE_SMALL &&
        page_size != VMEM_PAGE_SIZE_LARGE &&
        page_size != VMEM_PAGE_SIZE_HUGE) {
        return true;
    }

    if (!is_aligned((u64)vaddr, page_size))
        return true;

    const u64 pml4e = AMD64_KPAGING_GET_PE(vaddr, 39);
    const u64 pdpe = AMD64_KPAGING_GET_PE(vaddr, 30);
    const u64 pde = AMD64_KPAGING_GET_PE(vaddr, 21);
    const u64 pte = AMD64_KPAGING_GET_PE(vaddr, 12);

    u64* pml4 = AMD64_GET_PML4_VIRT();
    if (!AMD64_KPAGING_CHECK_ENTRY(pml4, pml4e))
        return false;

    u64* pdpt = AMD64_GET_PDPT_VIRT(pml4e);
    if (!AMD64_KPAGING_CHECK_ENTRY(pdpt, pdpe))
        return false;

    if ((pdpt[pdpe] & AMD64_PTE_HUGE) || page_size >= VMEM_PAGE_SIZE_HUGE)
        return true;

    u64* pd = AMD64_GET_PDT_VIRT(pml4e, pdpe);
    if (!AMD64_KPAGING_CHECK_ENTRY(pd, pde))
        return false;

    if ((pd[pde] & AMD64_PTE_HUGE) || page_size >= VMEM_PAGE_SIZE_LARGE)
        return true;

    u64* pt = AMD64_GET_PT_VIRT(pml4e, pdpe, pde);
    return AMD64_KPAGING_CHECK_ENTRY(pt, pte);
}

bool amd64_vmm_install_virtual_address(const void* vaddr, const void* paddr, u64 page_size, u64 flags) {
    if (amd64_vmm_page_already_mapped(vaddr, page_size))
        return false;

    if (!is_aligned((u64)vaddr, page_size) ||
        !is_aligned((u64)paddr, page_size))
        return false;

    const bool user = (flags & AMD64_PTE_USER) != 0;
    const u64 pml4e = AMD64_KPAGING_GET_PE(vaddr, 39);
    const u64 pdpe = AMD64_KPAGING_GET_PE(vaddr, 30);
    const u64 pde = AMD64_KPAGING_GET_PE(vaddr, 21);
    const u64 pte = AMD64_KPAGING_GET_PE(vaddr, 12);

    u64* pdpt = vmm_write_next_table(AMD64_GET_PML4_VIRT(), pml4e, AMD64_GET_PDPT_VIRT(pml4e), user);
    if (!pdpt)
        return false;

    if (page_size == VMEM_PAGE_SIZE_HUGE)
        return vmm_write_mapping(pdpt, pdpe, vaddr, paddr, flags, page_size);

    u64* pd = vmm_write_next_table(pdpt, pdpe, AMD64_GET_PDT_VIRT(pml4e, pdpe), user);
    if (!pd)
        return false;

    if (page_size == VMEM_PAGE_SIZE_LARGE)
        return vmm_write_mapping(pd, pde, vaddr, paddr, flags, page_size);

    u64* pt = vmm_write_next_table(pd, pde, AMD64_GET_PT_VIRT(pml4e, pdpe, pde), user);
    if (!pt)
        return false;

    return vmm_write_mapping(pt, pte, vaddr, paddr, flags, page_size);
}

bool amd64_vmm_remove_mapping(const void* vaddr, u64 page_size, u64* old_paddr) {
    if (page_size != VMEM_PAGE_SIZE_SMALL &&
        page_size != VMEM_PAGE_SIZE_LARGE &&
        page_size != VMEM_PAGE_SIZE_HUGE)
        return false;

    if (!is_aligned((u64)vaddr, page_size))
        return false;

    const u64 pml4e = AMD64_KPAGING_GET_PE(vaddr, 39);
    const u64 pdpe = AMD64_KPAGING_GET_PE(vaddr, 30);
    const u64 pde = AMD64_KPAGING_GET_PE(vaddr, 21);
    const u64 pte = AMD64_KPAGING_GET_PE(vaddr, 12);

    u64* pdpt = amd64_vmm_get_table(AMD64_GET_PML4_VIRT(), pml4e, AMD64_GET_PDPT_VIRT(pml4e));
    if (!pdpt)
        return false;

    if (page_size == VMEM_PAGE_SIZE_HUGE)
        return vmm_clear_mapping(pdpt, pdpe, vaddr, true, VMEM_PAGE_SIZE_HUGE, old_paddr);

    u64* pd = amd64_vmm_get_table(pdpt, pdpe, AMD64_GET_PDT_VIRT(pml4e, pdpe));
    if (!pd)
        return false;

    if (page_size == VMEM_PAGE_SIZE_LARGE)
        return vmm_clear_mapping(pd, pde, vaddr, true, VMEM_PAGE_SIZE_LARGE, old_paddr);

    u64* pt = amd64_vmm_get_table(pd, pde, AMD64_GET_PT_VIRT(pml4e, pdpe, pde));
    if (!pt)
        return false;

    return vmm_clear_mapping(pt, pte, vaddr, false, VMEM_PAGE_SIZE_SMALL, old_paddr);
}