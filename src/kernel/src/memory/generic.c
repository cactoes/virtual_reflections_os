#include "memory/generic.h"
#include "memory/pmm.h"
#include "libmem.h"
#include "libdef.h"

// TODO @since 24/11/2026 -- 00:30
// spinlocks for heaps

static virtual_heap_t virtual_kernel_heap;
static virtual_heap_t virtual_kernel_mmio_heap;

#include "sync/spinlock.h"

static spinlock_t global_lock;

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

static
u64 virtual_alloc_collect_unused(virtual_heap_t* heap, virtual_heap_block_t* out[], u64 capacity) {
    if (!heap || !heap->table || !out || capacity == 0)
        return 0;

    for (u64 i = 0; i < capacity; i++)
        out[i] = nullptr;

    u64 count = 0;
    for (virtual_heap_block_table_t* table = heap->table;
         table && count < capacity;
         table = table->next) {
        const u64 slot_count = sizeof(table->blocks) / sizeof(table->blocks[0]);

        for (u64 i = 0; i < slot_count && count < capacity; i++) {
            virtual_heap_block_t* block = &table->blocks[i];

            if (HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA |
                                      VIRTUAL_HEAP_BLOCK_ACCESSED))
                continue;

            out[count++] = block;
        }
    }

    return count;
}

static
void virtual_alloc_link_after(virtual_heap_block_t* previous, virtual_heap_block_t* node) {
    node->prev = previous;
    node->next = previous->next;

    if (previous->next)
        previous->next->prev = node;

    previous->next = node;
}

void* virtual_alloc_aligned(virtual_heap_t* virtual_heap, u64 size, u64 alignment) {
    if (!virtual_heap || !virtual_heap->table || size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0)
        return nullptr;

    const u64 allocation_size = align_up(size, 16);
    if (allocation_size < size)
        return nullptr;

    virtual_heap_block_t* spare[2];
    const u64 spare_count = virtual_alloc_collect_unused(virtual_heap, spare, 2);

    virtual_heap_block_t* donor = nullptr;
    u64 allocation_start = 0;
    u64 allocation_end = 0;
    u64 head_size = 0;
    u64 tail_size = 0;

    for (virtual_heap_block_t* block = &virtual_heap->table->blocks[0]; block; block = block->next) {
        if (!HAS_ALL(block->flags, VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE) || HAS_ANY(block->flags, VIRTUAL_HEAP_BLOCK_METADATA))
            continue;

        const u64 block_start = (u64)block->start;
        if (block->size > (~(u64)0) - block_start)
            continue;

        const u64 block_end = block_start + block->size;
        const u64 aligned_start = align_up(block_start, alignment);
        if (aligned_start < block_start || aligned_start > block_end)
            continue;

        if (allocation_size > block_end - aligned_start)
            continue;

        const u64 aligned_end = aligned_start + allocation_size;
        const u64 candidate_head = aligned_start - block_start;
        const u64 candidate_tail = block_end - aligned_end;
        const u64 descriptors_needed =
            (candidate_head != 0 ? 1 : 0) +
            (candidate_tail != 0 ? 1 : 0);

        if (descriptors_needed > spare_count)
            continue;

        donor = block;
        allocation_start = aligned_start;
        allocation_end = aligned_end;
        head_size = candidate_head;
        tail_size = candidate_tail;
        break;
    }

    if (!donor) {
        // TODO @since 21/11/2026 -- 02:14
        // grow heap
        return nullptr;
    }

    u64 spare_index = 0;
    virtual_heap_block_t* allocated_block;

    if (head_size == 0) {
        allocated_block = donor;
        allocated_block->size = allocation_size;
        CLEAR_MASK(allocated_block->flags, VIRTUAL_HEAP_BLOCK_FREE);
    } else {
        donor->size = head_size;

        allocated_block = spare[spare_index++];
        allocated_block->start = (void*)allocation_start;
        allocated_block->size = allocation_size;
        allocated_block->flags = VIRTUAL_HEAP_BLOCK_ACCESSED;
        virtual_alloc_link_after(donor, allocated_block);
    }

    if (tail_size != 0) {
        virtual_heap_block_t* tail_block = spare[spare_index++];
        tail_block->start = (void*)allocation_end;
        tail_block->size = tail_size;
        tail_block->flags = VIRTUAL_HEAP_BLOCK_ACCESSED | VIRTUAL_HEAP_BLOCK_FREE;
        virtual_alloc_link_after(allocated_block, tail_block);
    }

    return (void*)allocation_start;
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

    virtual_heap_block_t* block = virtual_find_block(virtual_heap, ptr);
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
    const u64 flags = spinlock_lock_irqsave(&global_lock);
    void* r = virtual_alloc(&virtual_kernel_heap, size);
    spinlock_unlock_irqrestore(&global_lock, flags);
    return r;
}

void* kalloc_aligned(u64 size, u64 align) {
    const u64 flags = spinlock_lock_irqsave(&global_lock);
    void* r = virtual_alloc_aligned(&virtual_kernel_heap, size, align);
    spinlock_unlock_irqrestore(&global_lock, flags);
    return r;
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