#include "memory/generic.h"
#include "memory/pmm.h"
#include "libmem.h"
#include "libdef.h"

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

bool donor_block_filter(const virtual_heap_block_t* block, const void* arg) {
    if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA))
        return false;

    return HAS_ALL(block->flags, VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE)
        && block->size >= *(const u64*)arg;
}

bool unused_block_filter(const virtual_heap_block_t* block, const void* arg) {
    (void)arg;

    // never reuse a descriptor that tracks a table page
    if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA))
        return false;

    // a descriptor slot that is not in use
    return !HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_ACCESSED);
}

bool filter_heap_blocks(virtual_heap_t* heap, void* arg, heap_block_filter_t callbacks[], u64 callback_count, virtual_heap_block_t* out_blocks[], u64 out_count) {
    if (!heap || !heap->table || !callbacks || !out_blocks ||
        callback_count == 0 || callback_count != out_count)
        return false;

    for (u64 i = 0; i < callback_count; i++)
        out_blocks[i] = nullptr;

    u64 remaining = callback_count;

    for (virtual_heap_block_table_t* table = heap->table; table && remaining; table = table->next) {
        const u64 slot_count = sizeof(table->blocks) / sizeof(table->blocks[0]);

        for (u64 s = 0; s < slot_count && remaining; s++) {
            virtual_heap_block_t* block = &table->blocks[s];

            for (u64 i = 0; i < callback_count; i++) {
                if (out_blocks[i] || !callbacks[i] || !callbacks[i](block, arg))
                    continue;

                out_blocks[i] = block;
                remaining--;
                break;
            }
        }
    }

    return remaining == 0;
}

void* virtual_alloc(virtual_heap_t* virtual_heap, u64 size) {
    if (size == 0)
        return nullptr;

    const u64 aligned = align_up(size, 16);
    if (aligned < size)
        return nullptr;

    size = aligned;

    heap_block_filter_t filters[] = { donor_block_filter, unused_block_filter };
    virtual_heap_block_t* blocks[] = { nullptr, nullptr };
    filter_heap_blocks(virtual_heap, &size, filters, 2, blocks, 2);

    virtual_heap_block_t* donor_block = blocks[0];
    if (!donor_block)
        // TODO @since 21/11/2026 -- 02:14
        // grow heap
        return nullptr;

    if (donor_block->size == size) {
        CLEAR_MASK(donor_block->flags, VIRTUAL_HEAP_BLOCK_FREE);
        return donor_block->start;
    }

    virtual_heap_block_t* unused_block = blocks[1];
    if (!unused_block)
        // TODO @since 21/11/2026 -- 02:14
        // grow table
        return nullptr;

    donor_block->size -= size;

    unused_block->start = (void*)((u64)donor_block->start + donor_block->size);
    unused_block->size  = size;
    unused_block->flags = VIRTUAL_HEAP_BLOCK_ACCESSED;

    unused_block->prev = donor_block;
    unused_block->next = donor_block->next;
    if (donor_block->next)
        donor_block->next->prev = unused_block;
    donor_block->next = unused_block;

    return unused_block->start;
}

static
bool block_is_free(const virtual_heap_block_t* block) {
    return block && HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_FREE) && !HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA);
}

static
bool blocks_adjacent(const virtual_heap_block_t* a, const virtual_heap_block_t* b) {
    return (u64)a->start + a->size == (u64)b->start;
}

static
void block_absorb_next(virtual_heap_block_t* block) {
    virtual_heap_block_t* next = block->next;

    block->size += next->size;
    block->next = next->next;
    if (next->next)
        next->next->prev = block;

    memzero(next, sizeof(virtual_heap_block_t));
}

bool virtual_free(virtual_heap_t* virtual_heap, void* ptr) {
    if (!ptr)
        return false;

    virtual_heap_block_t* block = &virtual_heap->table->blocks[0];
    while (block && block->start != ptr)
        block = block->next;

    if (!block)
        return false;

    if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA))
        return false;

    if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_FREE))
        return false;

    SET_MASK(block->flags, VIRTUAL_HEAP_BLOCK_FREE);

    if (block_is_free(block->next) && blocks_adjacent(block, block->next))
        block_absorb_next(block);

    virtual_heap_block_t* prev = block->prev;
    if (block_is_free(prev) && blocks_adjacent(prev, block))
        block_absorb_next(prev);

    return true;
}