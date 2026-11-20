#include "memory/generic.h"
#include "memory/pmm.h"
#include "libmem.h"

static
u64 pick_page_size(u64 vaddr, u64 remaining) {
    if (is_aligned(vaddr, VMEM_PAGE_SIZE_HUGE) && remaining >= VMEM_PAGE_SIZE_HUGE)
        return VMEM_PAGE_SIZE_HUGE;

    if (is_aligned(vaddr, VMEM_PAGE_SIZE_LARGE) && remaining >= VMEM_PAGE_SIZE_LARGE)
        return VMEM_PAGE_SIZE_LARGE;

    return VMEM_PAGE_SIZE_SMALL;
}

static
bool virtual_heap_map_range(u64 vaddr, u64 remaining, u64 flags) {
    if (remaining == 0)
        return true;

    const u64 page_size = pick_page_size(vaddr, remaining);
    const u64 frame_count = page_size / VMEM_PAGE_SIZE_SMALL;

    void* paddr = pmm_alloc_pages_aligned(frame_count, page_size);
    if (!paddr)
        return false;

    bool ok;
    switch (page_size) {
        case VMEM_PAGE_SIZE_HUGE: ok = vmm_map_1gb((void*)vaddr, paddr, flags); break;
        case VMEM_PAGE_SIZE_LARGE: ok = vmm_map_2mb((void*)vaddr, paddr, flags); break;
        case VMEM_PAGE_SIZE_SMALL: ok = vmm_map_4kb((void*)vaddr, paddr, flags); break;
        default: ok = false; break;
    }

    if (!ok) {
        pmm_free_pages(paddr, frame_count);
        return false;
    }

    if (!virtual_heap_map_range(vaddr + page_size, remaining - page_size, flags)) {
        switch (page_size) {
            case VMEM_PAGE_SIZE_HUGE: vmm_unmap_1gb((void*)vaddr, nullptr); break;
            case VMEM_PAGE_SIZE_LARGE: vmm_unmap_2mb((void*)vaddr, nullptr); break;
            case VMEM_PAGE_SIZE_SMALL: vmm_unmap_4kb((void*)vaddr, nullptr); break;
            default: break;
        }
        pmm_free_pages(paddr, frame_count);
        return false;
    }

    return true;
}

bool virtual_heap_create(void* vaddr_start, void* vaddr_limit, u64 start_size, virtual_heap_t* virtual_heap, u64 flags) {
    if (!(flags & VMEM_WRITE))
        return false;

    const u64 start = (u64)vaddr_start;
    const u64 end = (u64)vaddr_limit;

    if (!virtual_heap || start >= end)
        return false;

    if (!is_aligned(start, VMEM_PAGE_SIZE_SMALL) ||
        !is_aligned(end, VMEM_PAGE_SIZE_SMALL) ||
        !is_aligned(start_size, VMEM_PAGE_SIZE_SMALL))
        return false;

    if (start_size < 2 * VMEM_PAGE_SIZE_SMALL || start_size > end - start)
        return false;

    memzero(virtual_heap, sizeof(virtual_heap_t));
    virtual_heap->heap_start = start;
    virtual_heap->heap_limit = end;

    if (!virtual_heap_map_range(start, start_size, flags))
        return false;

    const u64 mapped = start_size;

    virtual_heap->table = (virtual_heap_block_table_t*)start;
    memzero(virtual_heap->table, sizeof(virtual_heap_block_table_t));

    virtual_heap_block_t* blocks = virtual_heap->table->blocks;

    // heap block table
    blocks[0].start = (void*)start;
    blocks[0].size = VMEM_PAGE_SIZE_SMALL;
    blocks[0].prev = nullptr;
    blocks[0].next = &blocks[1];
    blocks[0].flags = VIRTUAL_HEAP_BLOCK_METADATA | VIRTUAL_HEAP_BLOCK_ACCESSED;

    // first actual block
    blocks[1].start = (void*)(start + VMEM_PAGE_SIZE_SMALL);
    blocks[1].size = mapped - VMEM_PAGE_SIZE_SMALL;
    blocks[1].prev = &blocks[0];
    blocks[1].next = nullptr;
    blocks[1].flags = VIRTUAL_HEAP_BLOCK_FREE | VIRTUAL_HEAP_BLOCK_ACCESSED;

    virtual_heap->heap_edge = start + mapped;
    virtual_heap->heap_size_current = mapped;

    return true;
}

void* virtual_alloc(virtual_heap_t* virtual_heap, u64 size) {
    // align size up to 16
    // reject 0
    // find first or best free block
    // none -> return nullptr (or grow heap)
    // exact fit -> return it
    // needs splitting -> get a spare descriptor
    //      none -> return nullptr (or grow table)
    //      else -> split donor from then end, fix prev/next & return it
    virtual_heap;
    size;
    return false;
}

void virtual_free(virtual_heap_t* virtual_heap, void* ptr) {
    virtual_heap;
    ptr;
}