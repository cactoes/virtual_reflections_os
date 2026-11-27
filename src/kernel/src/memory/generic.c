#include "memory/generic.h"
#include "memory/pmm.h"
#include "libmem.h"
#include "libdef.h"

// TODO @since 24/11/2026 -- 00:30
// spinlocks for heaps

static virtual_heap_t virtual_kernel_heap;
static virtual_heap_t virtual_kernel_mmio_heap;

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

static
virtual_heap_block_t* virtual_find_block(virtual_heap_t* heap, void* ptr) {
    if (!heap || !heap->table)
        return nullptr;

    virtual_heap_block_t* block = &heap->table->blocks[0];
    while (block && block->start != ptr)
        block = block->next;
    return block;
}

static
bool virtual_heap_bounds_valid(const virtual_heap_t* heap) {
    if (heap->heap_start >= heap->heap_limit || heap->heap_edge < heap->heap_start || heap->heap_edge > heap->heap_limit)
        return false;

    return (u64)heap->table == heap->heap_start &&
           is_aligned(heap->heap_start, VMEM_PAGE_SIZE_SMALL) &&
           is_aligned(heap->heap_limit, VMEM_PAGE_SIZE_SMALL) &&
           is_aligned(heap->heap_edge, VMEM_PAGE_SIZE_SMALL) &&
           heap->heap_edge - heap->heap_start >= 2 * VMEM_PAGE_SIZE_SMALL &&
           heap->heap_size_current == heap->heap_edge - heap->heap_start;
}

static
heap_map_result_t virtual_heap_map_growth(virtual_heap_t* heap, u64 size) {
    if (!virtual_heap_bounds_valid(heap) || !(heap->mapping_flags & VMEM_WRITE) || !is_aligned(size, VMEM_PAGE_SIZE_SMALL))
        return HEAP_MAP_FAILED;

    const u64 start = heap->heap_edge;

    if (size > heap->heap_limit - start)
        return HEAP_MAP_FAILED;

    u64 mapped = 0;
    while (mapped < size) {
        void* frame = pmm_alloc_pages(1);
        if (!frame)
            break;

        if (!vmm_map_4kb((void*)(start + mapped), frame, heap->mapping_flags)) {
            pmm_free_pages(frame, 1);
            break;
        }

        mapped += VMEM_PAGE_SIZE_SMALL;
    }

    if (mapped == size)
        return HEAP_MAP_OK;

    bool rollback_ok = true;
    while (mapped != 0) {
        mapped -= VMEM_PAGE_SIZE_SMALL;
        u64 old_paddr = 0;
        if (vmm_unmap_4kb((void*)(start + mapped), &old_paddr))
            pmm_free_pages((void*)old_paddr, 1);
        else
            rollback_ok = false;
    }

    return rollback_ok ? HEAP_MAP_FAILED : HEAP_MAP_ROLLBACK_FAILED;
}

static
virtual_heap_block_t* virtual_heap_tail(virtual_heap_t* heap) {
    virtual_heap_block_t* tail = &heap->table->blocks[0];
    while (tail->next)
        tail = tail->next;

    return tail;
}

static
bool virtual_heap_tail_is_free(const virtual_heap_block_t* tail) {
    return HAS_ALL(tail->flags, VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE) && !HAS_ANY(tail->flags, VIRTUAL_HEAP_BLOCK_METADATA);
}

static
void virtual_alloc_link_after(virtual_heap_block_t* previous, virtual_heap_block_t* node) {
    node->prev = previous;
    node->next = previous->next;

    if (previous->next)
        previous->next->prev = node;

    previous->next = node;
}

static
u64 virtual_alloc_collect_unused(virtual_heap_t* heap, virtual_heap_block_t* out[], u64 capacity) {
    if (!out || capacity == 0)
        return 0;

    for (u64 i = 0; i < capacity; i++)
        out[i] = nullptr;

    u64 count = 0;
    for (virtual_heap_block_table_t* table = heap->table; table && count < capacity; table = table->next) {
        const u64 slot_count = sizeof(table->blocks) / sizeof(table->blocks[0]);

        for (u64 i = 0; i < slot_count && count < capacity; i++) {
            virtual_heap_block_t* block = &table->blocks[i];

            if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA | VIRTUAL_HEAP_BLOCK_ACCESSED))
                continue;

            out[count++] = block;
        }
    }

    return count;
}

static
bool virtual_heap_add_table(virtual_heap_t* heap) {
    if (!virtual_heap_bounds_valid(heap))
        return false;

    const u64 start = heap->heap_edge;
    if (VMEM_PAGE_SIZE_SMALL > heap->heap_limit - start)
        return false;

    virtual_heap_block_t* tail = virtual_heap_tail(heap);
    const u64 tail_start = (u64)tail->start;
    if (tail_start < heap->heap_start || tail_start > start ||
        tail->size != start - tail_start)
        return false;

    virtual_heap_block_table_t* last_table = heap->table;
    while (last_table->next)
        last_table = last_table->next;

    if (virtual_heap_map_growth(heap, VMEM_PAGE_SIZE_SMALL) != HEAP_MAP_OK)
        return false;

    virtual_heap_block_table_t* table = (void*)start;
    memzero(table, VMEM_PAGE_SIZE_SMALL);
    virtual_heap_block_t* metadata = &table->blocks[0];
    metadata->start = (void*)start;
    metadata->size = VMEM_PAGE_SIZE_SMALL;
    metadata->flags = VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_METADATA;
    virtual_alloc_link_after(tail, metadata);

    table->prev = last_table;
    last_table->next = table;
    heap->heap_edge = start + VMEM_PAGE_SIZE_SMALL;
    heap->heap_size_current += VMEM_PAGE_SIZE_SMALL;
    return true;
}

static
bool virtual_heap_grow(virtual_heap_t* heap, u64 allocation_size, u64 alignment, u64 spare_count) {
    if (!virtual_heap_bounds_valid(heap) || allocation_size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0)
        return false;

    if (spare_count < 2)
        return virtual_heap_add_table(heap);

    const u64 edge = heap->heap_edge;
    if (edge == heap->heap_limit)
        return false;

    virtual_heap_block_t* tail = virtual_heap_tail(heap);
    const u64 tail_start = (u64)tail->start;
    if (tail_start < heap->heap_start || tail_start > edge ||
        tail->size != edge - tail_start)
        return false;

    const bool extend_tail = virtual_heap_tail_is_free(tail);
    if (!extend_tail && spare_count < 3)
        return virtual_heap_add_table(heap);

    const u64 candidate_start = extend_tail ? tail_start : edge;
    const u64 aligned_start = align_up(candidate_start, alignment);
    if (aligned_start < candidate_start || aligned_start > heap->heap_limit || allocation_size > heap->heap_limit - aligned_start)
        return false;

    const u64 required_end = aligned_start + allocation_size;
    if (required_end <= edge)
        return false;

    const u64 needed = required_end - edge;
    const u64 minimum = align_up(needed, VMEM_PAGE_SIZE_SMALL);
    const u64 available = heap->heap_limit - edge;
    if (minimum < needed || minimum > available)
        return false;

    virtual_heap_block_t* new_free = nullptr;
    if (!extend_tail) {
        virtual_heap_block_t* spare[1];
        if (virtual_alloc_collect_unused(heap, spare, 1) != 1)
            return false;

        new_free = spare[0];
    }

    u64 growth = minimum;
    if (growth < VMEM_PAGE_SIZE_LARGE)
        growth = VMEM_PAGE_SIZE_LARGE;
    if (growth > available)
        growth = available;

    heap_map_result_t result = virtual_heap_map_growth(heap, growth);
    if (result == HEAP_MAP_FAILED && growth != minimum) {
        growth = minimum;
        result = virtual_heap_map_growth(heap, growth);
    }
    if (result != HEAP_MAP_OK)
        return false;

    if (extend_tail) {
        tail->size += growth;
    } else {
        memzero(new_free, sizeof(*new_free));
        new_free->start = (void*)edge;
        new_free->size = growth;
        new_free->flags = VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE;
        virtual_alloc_link_after(tail, new_free);
    }
    heap->heap_edge = edge + growth;
    heap->heap_size_current += growth;
    return true;
}

bool virtual_heap_create(void* vaddr_start, void* vaddr_limit, u64 start_size, virtual_heap_t* virtual_heap, u64 flags) {
    if (!(flags & VMEM_WRITE))
        return false;

    const u64 start = (u64)vaddr_start;
    const u64 end = (u64)vaddr_limit;
    if (start >= end)
        return false;

    if (!is_aligned(start, VMEM_PAGE_SIZE_SMALL) ||
        !is_aligned(end, VMEM_PAGE_SIZE_SMALL) ||
        !is_aligned(start_size, VMEM_PAGE_SIZE_SMALL))
        return false;

    if (start_size < 2 * VMEM_PAGE_SIZE_SMALL || start_size > end - start)
        return false;

    memzero(virtual_heap, sizeof(virtual_heap_t));
    virtual_heap->mapping_flags = flags;
    virtual_heap->heap_start = start;
    virtual_heap->heap_limit = end;
    if (!virtual_heap_map_range(start, start_size, flags))
        return false;

    virtual_heap->table = (virtual_heap_block_table_t*)start;
    memzero(virtual_heap->table, sizeof(virtual_heap_block_table_t));
    virtual_heap_block_t* blocks = virtual_heap->table->blocks;

    blocks[0].start = (void*)start;
    blocks[0].size = VMEM_PAGE_SIZE_SMALL;
    blocks[0].prev = nullptr;
    blocks[0].next = &blocks[1];
    blocks[0].flags = VIRTUAL_HEAP_BLOCK_METADATA | VIRTUAL_HEAP_BLOCK_ACCESSED;

    blocks[1].start = (void*)(start + VMEM_PAGE_SIZE_SMALL);
    blocks[1].size = start_size - VMEM_PAGE_SIZE_SMALL;
    blocks[1].prev = &blocks[0];
    blocks[1].next = nullptr;
    blocks[1].flags = VIRTUAL_HEAP_BLOCK_FREE | VIRTUAL_HEAP_BLOCK_ACCESSED;

    virtual_heap->heap_edge = start + start_size;
    virtual_heap->heap_size_current = start_size;
    return true;
}

static
bool donor_block_filter(const virtual_heap_block_t* block, const void* arg) {
    if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA))
        return false;

    return HAS_ALL(block->flags, VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE) 
            && block->size >= *(const u64*)arg;
}

static
bool unused_block_filter(const virtual_heap_block_t* block, const void* arg) {
    (void)arg;

    if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA))
        return false;

    return !HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_ACCESSED);
}

static
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

void* virtual_alloc_aligned(virtual_heap_t* virtual_heap, u64 size, u64 alignment) {
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0)
        return nullptr;

    const u64 allocation_size = align_up(size, 16);
    if (allocation_size < size)
        return nullptr;

    spinlock_lock(&virtual_heap->lock);

    if (!virtual_heap_bounds_valid(virtual_heap) || allocation_size > virtual_heap->heap_limit - virtual_heap->heap_start) {
        spinlock_unlock(&virtual_heap->lock);
        return nullptr;
    }

    for (;;) {
        virtual_heap_block_t* spare[3];
        const u64 spare_count = virtual_alloc_collect_unused(virtual_heap, spare, 3);
        virtual_heap_block_t* donor = nullptr;
        u64 allocation_start = 0, allocation_end = 0;
        u64 head_size = 0, tail_size = 0;

        for (virtual_heap_block_t* block = &virtual_heap->table->blocks[0]; block; block = block->next) {
            if (!HAS_ALL(block->flags, VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE) || HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA))
                continue;

            const u64 block_start = (u64)block->start;

            if (block_start < virtual_heap->heap_start || block_start > virtual_heap->heap_edge ||
                block->size > virtual_heap->heap_edge - block_start)
                continue;

            const u64 block_end = block_start + block->size;
            const u64 aligned_start = align_up(block_start, alignment);
            if (aligned_start < block_start || aligned_start > block_end ||
                allocation_size > block_end - aligned_start)
                continue;

            const u64 aligned_end = aligned_start + allocation_size;
            const u64 candidate_head = aligned_start - block_start;
            const u64 candidate_tail = block_end - aligned_end;
            const u64 needed_slots = (candidate_head != 0 ? 1 : 0) + (candidate_tail != 0 ? 1 : 0);
            if (needed_slots > spare_count)
                continue;

            donor = block;
            allocation_start = aligned_start;
            allocation_end = aligned_end;
            head_size = candidate_head;
            tail_size = candidate_tail;
            break;
        }

        if (!donor) {
            if (virtual_heap_grow(virtual_heap, allocation_size, alignment, spare_count))
                continue;

            spinlock_unlock(&virtual_heap->lock);
            return nullptr;
        }

        u64 spare_index = 0;
        virtual_heap_block_t* allocated;
        if (head_size == 0) {
            allocated = donor;
            allocated->size = allocation_size;
            CLEAR_MASK(allocated->flags, VIRTUAL_HEAP_BLOCK_FREE);
        } else {
            donor->size = head_size;
            allocated = spare[spare_index++];
            allocated->start = (void*)allocation_start;
            allocated->size = allocation_size;
            allocated->flags = VIRTUAL_HEAP_BLOCK_ACCESSED;
            virtual_alloc_link_after(donor, allocated);
        }

        if (tail_size != 0) {
            virtual_heap_block_t* tail = spare[spare_index++];
            tail->start = (void*)allocation_end;
            tail->size = tail_size;
            tail->flags = VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE;
            virtual_alloc_link_after(allocated, tail);
        }

        spinlock_unlock(&virtual_heap->lock);
        return (void*)allocation_start;
    }
}

void* virtual_alloc(virtual_heap_t* virtual_heap, u64 size) {
    return virtual_alloc_aligned(virtual_heap, size, 16);
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

    spinlock_lock(&virtual_heap->lock);

    virtual_heap_block_t* block = virtual_find_block(virtual_heap, ptr);
    if (!block || HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA | VIRTUAL_HEAP_BLOCK_FREE)) {
        spinlock_unlock(&virtual_heap->lock);
        return false;
    }

    SET_MASK(block->flags, VIRTUAL_HEAP_BLOCK_FREE);

    if (block_is_free(block->next) && blocks_adjacent(block, block->next))
        block_absorb_next(block);

    virtual_heap_block_t* prev = block->prev;
    if (block_is_free(prev) && blocks_adjacent(prev, block))
        block_absorb_next(prev);

    spinlock_unlock(&virtual_heap->lock);
    return true;
}

static
u64 mmio_alignment_for(u64 size) {
    if (size >= VMEM_PAGE_SIZE_HUGE)
        return VMEM_PAGE_SIZE_HUGE;

    if (size >= VMEM_PAGE_SIZE_LARGE)
        return VMEM_PAGE_SIZE_LARGE;

    return VMEM_PAGE_SIZE_SMALL;
}

static
void mmio_unmap_pages(u64 vbase, u64 total, u64 upto) {
    u64 off = 0;
    while (off < upto) {
        const u64 page = pick_page_size(vbase + off, total - off);
        void* vaddr = (void*)(vbase + off);

        switch (page) {
            case VMEM_PAGE_SIZE_HUGE:  vmm_unmap_1gb(vaddr, nullptr); break;
            case VMEM_PAGE_SIZE_LARGE: vmm_unmap_2mb(vaddr, nullptr); break;
            default:                   vmm_unmap_4kb(vaddr, nullptr); break;
        }

        off += page;
    }
}

static
bool mmio_map_pages(u64 vbase, u64 pbase, u64 total, u64 flags) {
    u64 off = 0;
    while (off < total) {
        const u64 page = pick_page_size(vbase + off, total - off);
        void* vaddr = (void*)(vbase + off);
        void* paddr = (void*)(pbase + off);

        bool ok;
        switch (page) {
            case VMEM_PAGE_SIZE_HUGE: ok = vmm_map_1gb(vaddr, paddr, flags); break;
            case VMEM_PAGE_SIZE_LARGE: ok = vmm_map_2mb(vaddr, paddr, flags); break;
            default: ok = vmm_map_4kb(vaddr, paddr, flags); break;
        }

        if (!ok) {
            mmio_unmap_pages(vbase, total, off);
            return false;
        }

        off += page;
    }
    return true;
}

static
void mmio_link_after(virtual_heap_block_t* prev, virtual_heap_block_t* node) {
    node->prev = prev;
    node->next = prev->next;
    if (prev->next)
        prev->next->prev = node;
    prev->next = node;
}

static
bool mmio_find_placement(u64 start, u64 end, u64 size, u64 align, u64 phase, u64* out) {
    if (end <= start || end - start < size)
        return false;

    const u64 hi = end - size;
    const u64 r = hi & (align - 1);
    const u64 back = (r >= phase) ? (r - phase) : (r + align - phase);

    if (back > hi - start)
        return false;

    *out = hi - back;
    return true;
}

static
u64 mmio_collect_slots(virtual_heap_t* mmio, virtual_heap_block_t* out[], u64 count) {
    heap_block_filter_t filters[] = { unused_block_filter, unused_block_filter, unused_block_filter, unused_block_filter };
    filter_heap_blocks(mmio, nullptr, filters, count, out, count);

    u64 n = 0;
    for (u64 i = 0; i < count; i++)
        if (out[i])
            n++;

    return n;
}

static
virtual_heap_block_t* mmio_carve(virtual_heap_t* mmio, u64 size, u64 align, u64 phase) {
    virtual_heap_block_t* donor = nullptr;
    u64 a = 0;

    for (virtual_heap_block_t* b = &mmio->table->blocks[0]; b; b = b->next) {
        if (!HAS_ALL(b->flags, VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE) ||
            HAS_ANY(b->flags, VIRTUAL_HEAP_BLOCK_METADATA))
            continue;

        if (mmio_find_placement((u64)b->start, (u64)b->start + b->size, size, align, phase, &a)) {
            donor = b;
            break;
        }
    }

    if (!donor)
        return nullptr;

    const u64 head = a - (u64)donor->start;
    const u64 tail = ((u64)donor->start + donor->size) - (a + size);
    const u64 need = (head ? 1 : 0) + (tail ? 1 : 0);

    virtual_heap_block_t* spare[2];
    if (mmio_collect_slots(mmio, spare, 2) < need)
        return nullptr;

    u64 s = 0;
    virtual_heap_block_t* used = donor;
    virtual_heap_block_t* last = donor;

    if (head) {
        used = spare[s++];
        donor->size = head;

        used->start = (void*)a;
        used->size  = size;
        used->flags = VIRTUAL_HEAP_BLOCK_ACCESSED;
        mmio_link_after(donor, used);
        last = used;
    } else {
        donor->size = size;
        CLEAR_MASK(donor->flags, VIRTUAL_HEAP_BLOCK_FREE);
    }

    if (tail) {
        virtual_heap_block_t* tail_block = spare[s++];
        tail_block->start = (void*)(a + size);
        tail_block->size  = tail;
        tail_block->flags = VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE;
        mmio_link_after(last, tail_block);
    }

    return used;
}

static
bool mmio_grow(virtual_heap_t* mmio) {
    // TODO @since 28/11/2026 -- 00:04
    // check if we keep in bounds of the mmio virtual range

    virtual_heap_block_t* block = mmio_carve(mmio, VMEM_PAGE_SIZE_SMALL, VMEM_PAGE_SIZE_SMALL, 0);
    if (!block)
        return false;

    void* paddr = pmm_alloc_pages(1);
    const bool ok = paddr && vmm_map_4kb(block->start, paddr, VMEM_KERNEL_DATA);
    if (!ok) {
        if (paddr)
            pmm_free_pages(paddr, 1);

        virtual_free(mmio, block->start);
        return false;
    }

    virtual_heap_block_table_t* table = (virtual_heap_block_table_t*)block->start;
    memzero(table, sizeof(virtual_heap_block_table_t));
    SET_MASK(block->flags, VIRTUAL_HEAP_BLOCK_METADATA);

    virtual_heap_block_table_t* last = mmio->table;
    while (last->next)
        last = last->next;
    last->next  = table;
    table->prev = last;
    return true;
}

static
void mmio_ensure_slots(virtual_heap_t* mmio) {
    virtual_heap_block_t* tmp[4];
    if (mmio_collect_slots(mmio, tmp, 4) < 4)
        mmio_grow(mmio);
}

bool virtual_mmio_create(virtual_heap_t* mmio, void* vaddr_start, void* vaddr_limit) {
    const u64 start = (u64)vaddr_start;
    const u64 limit = (u64)vaddr_limit;

    if (!mmio || start >= limit ||
        !is_aligned(start, VMEM_PAGE_SIZE_SMALL) || !is_aligned(limit, VMEM_PAGE_SIZE_SMALL) ||
        limit - start < 2 * VMEM_PAGE_SIZE_SMALL)
        return false;

    void* paddr = pmm_alloc_pages(1);
    if (!paddr)
        return false;

    if (!vmm_map_4kb((void*)start, paddr, VMEM_KERNEL_DATA)) {
        pmm_free_pages(paddr, 1);
        return false;
    }

    memzero(mmio, sizeof(virtual_heap_t));
    mmio->heap_start = start;
    mmio->heap_limit = limit;
    mmio->table = (virtual_heap_block_table_t*)start;
    memzero(mmio->table, sizeof(virtual_heap_block_table_t));

    virtual_heap_block_t* blocks = mmio->table->blocks;

    blocks[0].start = (void*)start;
    blocks[0].size = VMEM_PAGE_SIZE_SMALL;
    blocks[0].prev = nullptr;
    blocks[0].next = &blocks[1];
    blocks[0].flags = VIRTUAL_HEAP_BLOCK_METADATA | VIRTUAL_HEAP_BLOCK_ACCESSED;

    blocks[1].start = (void*)(start + VMEM_PAGE_SIZE_SMALL);
    blocks[1].size = limit - start - VMEM_PAGE_SIZE_SMALL;
    blocks[1].prev = &blocks[0];
    blocks[1].next = nullptr;
    blocks[1].flags = VIRTUAL_HEAP_BLOCK_FREE | VIRTUAL_HEAP_BLOCK_ACCESSED;

    mmio->heap_edge = limit;
    mmio->heap_size_current = VMEM_PAGE_SIZE_SMALL;
    return true;
}

void* virtual_mmio_map_ex(virtual_heap_t* mmio, void* paddr, u64 size, u64 flags) {
    if (!mmio || !mmio->table || size == 0)
        return nullptr;

    const u64 offset = (u64)paddr & (VMEM_PAGE_SIZE_SMALL - 1);
    const u64 pbase = align_down((u64)paddr, VMEM_PAGE_SIZE_SMALL);
    const u64 total = offset + size;
    const u64 map_size = align_up(total, VMEM_PAGE_SIZE_SMALL);
    if (total < size || map_size < total)
        return nullptr;

    mmio_ensure_slots(mmio);

    const u64 align = mmio_alignment_for(map_size);
    virtual_heap_block_t* block = mmio_carve(mmio, map_size, align, pbase & (align - 1));
    if (!block)
        return nullptr;

    if (!mmio_map_pages((u64)block->start, pbase, map_size, flags)) {
        virtual_free(mmio, block->start);
        return nullptr;
    }

    return (void*)((u64)block->start + offset);
}

void* virtual_mmio_map(virtual_heap_t* mmio, void* paddr, u64 size) {
    return virtual_mmio_map_ex(mmio, paddr, size, VMEM_MMIO);
}

bool virtual_mmio_unmap(virtual_heap_t* mmio, void* ptr) {
    if (!mmio || !mmio->table || !ptr)
        return false;

    void* base = (void*)align_down((u64)ptr, VMEM_PAGE_SIZE_SMALL);

    virtual_heap_block_t* block = virtual_find_block(mmio, base);
    if (!block || HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA | VIRTUAL_HEAP_BLOCK_FREE))
        return false;

    mmio_unmap_pages((u64)block->start, block->size, block->size);
    return virtual_free(mmio, base);
}

bool kmem_bind(void* heap_start, void* heap_limit, u64 start_size) {
    return virtual_heap_create(heap_start,heap_limit, start_size, &virtual_kernel_heap, VMEM_KERNEL_DATA | VMEM_EXEC);
}

void* kalloc(u64 size) {
    return virtual_alloc(&virtual_kernel_heap, size);
}

void* kalloc_aligned(u64 size, u64 align) {
    return virtual_alloc_aligned(&virtual_kernel_heap, size, align);
}

void kfree(void* ptr) {
    (void)virtual_free(&virtual_kernel_heap, ptr);
}

bool kmmio_bind(void* heap_start, void* heap_limit) {
    return virtual_mmio_create(&virtual_kernel_mmio_heap, heap_start, heap_limit);
}

void* kmmio_map(void* paddr, u64 size) {
    return virtual_mmio_map(&virtual_kernel_mmio_heap, paddr, size);
}

void kmmio_unmap(void* ptr) {
    virtual_mmio_unmap(&virtual_kernel_mmio_heap, ptr);
}

void* __malloc_impl(u64 size) {
    return kalloc(size);
}

void __free_impl(void* ptr) {
    kfree(ptr);
}