#include "memory/heap.hpp"
#include "memory/paging.hpp"
#include "memory/vmem.hpp"

static heap_t* g_heap = nullptr;
static dma_heap_manager_t* global_dma_heap_manager = nullptr;

bool heap_block_filters::donor_block_filter(const heap_block_t* p_block, const void* p_param) {
    return p_block->used && p_block->free && p_block->size >= *(size_t*)p_param;
}

bool heap_block_filters::unused_block_filter(const heap_block_t* p_block, const void* p_param) {
    return !p_block->used;
}

bool heap_block_filters::last_block_filter(const heap_block_t* p_block, const void* p_param) {
    return p_block->used && !p_block->next;
}

int heap_filter_blocks(heap_t* p_heap, void* p_param, heap_block_filter_callback_t p_hbfc_array[], size_t bfc_array_size, heap_block_t* p_block_array[], size_t block_array_size) {
    if (bfc_array_size != block_array_size)
        return -1;

    size_t found_size = 0;
    for (size_t i = 0; i < p_heap->heap_block_array_size; i++) {
        if (found_size == block_array_size)
            break;
        
        heap_block_t* current_block = &p_heap->heap_block_array[i];

        for (size_t k = 0; k < bfc_array_size; k++) {
            heap_block_t** stored_block = &p_block_array[k];
            heap_block_filter_callback_t filter_func = p_hbfc_array[k];

            if (*stored_block == nullptr) {
                if (filter_func(current_block, p_param)) {
                    *stored_block = current_block;
                    found_size++;
                    break;
                }
            }
        }
    }

    return found_size;
}

bool heap_init(heap_t* heap, void* pml4, void* vaddr, size_t size, bool is_user) {
    // the heap is just raw memory no data structures
    uint64_t heap_size = vmem_smart_alloc_pages(pml4, (void*)((uint64_t)vaddr + PAGE_SIZE_LARGE), size, is_user);

    // heap struct
    void* p_page = pmem_get_page();
    while (p_page && !is_aligned((uint64_t)p_page, PAGE_SIZE_LARGE))
        p_page = pmem_get_page();

    if (p_page == nullptr)
        return false;

    if (!pmem_try_reserve_address((void*)((uint64_t)p_page + PAGE_SIZE), (PAGE_SIZE_LARGE / PAGE_SIZE - 1)))
        return false;

    if (!vmem_map_2mb(pml4, vaddr, p_page, is_user))
        return false;

    // setup heap stuct
    heap->heap_block_array = (heap_block_t*)vaddr;
    heap->heap_block_array_size = PAGE_SIZE_LARGE / sizeof(heap_block_t);
    heap->heap_block_count = 1;
    heap->start_virtual_addr = (void*)((uint64_t)vaddr + PAGE_SIZE_LARGE);
    heap->size = heap_size;
    heap->pml4 = pml4;

    // first block is size of entire heap, but un allocated
    heap->heap_block_array->start_real_addr = (void*)((uint64_t)vaddr + PAGE_SIZE_LARGE);
    heap->heap_block_array->next = nullptr;
    heap->heap_block_array->size = heap_size;
    heap->heap_block_array->free = true;
    heap->heap_block_array->used = true;

    return true;
}

bool heap_expand(heap_t* p_heap, size_t size) {
    heap_block_filter_callback_t filters[] = {
        heap_block_filters::last_block_filter,
        heap_block_filters::unused_block_filter
    };
    heap_block_t* blocks[HEAP_FILTERS_SIZE(filters)] = {};

    int result = heap_filter_blocks(p_heap, HEAP_MAKE_FILTER_PARAM(size), filters, HEAP_FILTERS_SIZE(filters), blocks, HEAP_FILTERS_SIZE(filters));

    if (result != HEAP_FILTERS_SIZE(filters))
        return false;

    void* heap_virtual_end = (void*)((uint64_t)p_heap->start_virtual_addr + p_heap->size);
    const auto new_heap_block_size = vmem_smart_alloc_pages(p_heap->pml4, heap_virtual_end, size);

    p_heap->size += new_heap_block_size;

    heap_block_t* new_block = blocks[1];
    new_block->used = true;
    new_block->free = true;
    new_block->size = new_heap_block_size;
    new_block->start_real_addr = heap_virtual_end;
    
    heap_block_t* last_block = blocks[0];
    last_block->next = new_block;

    return true;
}

void* heap_alloc(heap_t* p_heap, size_t size) {
    // not needed
    // if (size > PAGE_SIZE_LARGE)
    //     return nullptr;

    heap_block_filter_callback_t filters[] = {
        heap_block_filters::donor_block_filter,
        heap_block_filters::unused_block_filter
    };
    heap_block_t* blocks[HEAP_FILTERS_SIZE(filters)] = {};
    
    int result = heap_filter_blocks(p_heap, HEAP_MAKE_FILTER_PARAM(size), filters, HEAP_FILTERS_SIZE(filters), blocks, HEAP_FILTERS_SIZE(filters));

    // heap block that is >= requested size
    // aka the donor block
    heap_block_t* donor_block = blocks[0];

    // early return, we dont need the unused block here
    // we could exit if we dont have a donor block but not needed to check here
    if (donor_block && donor_block->size == size) {
        donor_block->free = false;
        donor_block->used = true;
        return donor_block->start_real_addr;
    }

    // heap block that we can write our data to
    // & assign our new memory block to
    heap_block_t* unused_block = blocks[1];

    // make sure we have both blocks
    // TODO @since 30/04/2025 -- 01:25
    // check this logic
    if (result != HEAP_FILTERS_SIZE(filters)) {
        if (!heap_expand(p_heap, size))
            return nullptr;

        return heap_alloc(p_heap, size);
    }

    donor_block->size -= size;

    unused_block->free = false;
    unused_block->used = true;
    unused_block->size = size;
    unused_block->start_real_addr = (void*)((uint64_t)donor_block->start_real_addr + donor_block->size);

    unused_block->next = donor_block->next;
    donor_block->next = unused_block;

    return unused_block->start_real_addr;
}

void heap_free(heap_t* p_heap, void* p_ptr) {
    for (heap_block_t* current_block = p_heap->heap_block_array; current_block; current_block = current_block->next) {
        // find the correct block & free it
        if (current_block->start_real_addr == p_ptr) {
            current_block->free = true;
            break;
        }
    }

    for (heap_block_t* current_block = p_heap->heap_block_array; current_block; current_block = current_block->next) {
        heap_block_t* next_block = current_block->next;
        if (current_block->free && next_block && next_block->free) {
            
            current_block->size += next_block->size;
            current_block->next = next_block->next;

            memzero((void*)next_block, sizeof(heap_block_t));
        }
    }
}

void* dma_heap_alloc(heap_t* p_dma_heap, size_t size, uint64_t align) {
    if (!is_aligned(size, align))
        return nullptr;

    heap_block_filter_callback_t filters[] = {
        heap_block_filters::donor_block_filter,
        heap_block_filters::unused_block_filter,
        heap_block_filters::unused_block_filter
    };
    heap_block_t* blocks[HEAP_FILTERS_SIZE(filters)] = {};
    
    const auto donor_block_size = size + align;
    int result = heap_filter_blocks(p_dma_heap, HEAP_MAKE_FILTER_PARAM(donor_block_size), filters, HEAP_FILTERS_SIZE(filters), blocks, HEAP_FILTERS_SIZE(filters));

    // heap block that is >= requested size
    // aka the donor block
    heap_block_t* donor_block = blocks[0];

    // early return, we dont need the unused block here
    // we could exit if we dont have a donor block but not needed to check here
    // we make sure the alignment & size checks out if so we can just return it here
    if (donor_block && donor_block->size == size
        && is_aligned((uint64_t)donor_block->start_real_addr, align)
        // not sure if we need to check the physical here
        /* && mem_is_aligned(dma_get_physical(dma_heap, donor_block->start_real_addr), align) */) {
        
        donor_block->free = false;
        donor_block->used = true;
        return donor_block->start_real_addr;
    }

    // make sure we have all blocks
    // if not we cant expand the heap so just leave it be
    if (result != HEAP_FILTERS_SIZE(filters))
        return nullptr;

    // heap block that we can write our data to
    // & assign our new memory block to
    heap_block_t* unused_block = blocks[1];

    // heap block that we can use to make the unused block our memory aligner
    heap_block_t* filler_block = blocks[2];

    donor_block->size -= size + align;

    unused_block->free = false;
    unused_block->used = true;
    unused_block->size = size + align;
    unused_block->start_real_addr = (void*)((uint64_t)donor_block->start_real_addr + donor_block->size);

    unused_block->next = donor_block->next;
    donor_block->next = unused_block;

    if (!is_aligned((uint64_t)unused_block->start_real_addr, align)) {
        uint64_t misalignment = (uint64_t)unused_block->start_real_addr % align;
        uint64_t correction = (align - misalignment) % align;
        
        // if (donor_block->size < correction)
        //     return nullptr;

        // donor_block->size -= correction;

        filler_block->free = true;
        filler_block->size = correction;
        // filler_block->start_real_addr = (void*)((uint64_t)donor_block->start_real_addr + donor_block->size);
        filler_block->start_real_addr = unused_block->start_real_addr;
        filler_block->used = true;

        unused_block->start_real_addr = (void*)((uint64_t)filler_block->start_real_addr + correction);
        unused_block->size -= correction;

        donor_block->next = filler_block;
        filler_block->next = unused_block;
    }

    return unused_block->start_real_addr;
}

void dma_heap_free(heap_t* p_dma_heap, void* p_block) {
    for (heap_block_t* current_block = p_dma_heap->heap_block_array; current_block; current_block = current_block->next) {
        // find the correct block & free it
        if (current_block->start_real_addr == p_block) {
            current_block->free = true;
            break;
        }
    }

    for (heap_block_t* current_block = p_dma_heap->heap_block_array; current_block; current_block = current_block->next) {
        heap_block_t* next_block = current_block->next;
        if (current_block->free && next_block && next_block->free) {
            current_block->size += next_block->size;
            current_block->next = next_block->next;

            memzero((void*)next_block, sizeof(heap_block_t));
        }
    }
}

uint64_t dma_get_physical(heap_t* p_dma_heap, void* p_block) {
    // return (uint64_t)vmem_virtual_to_physical(p_dma_heap->pml4, p_block);
    return (uint64_t)vmem_virtual_to_physical(p_block);
}

uint32_t dma_get_physical_lower(heap_t* p_dma_heap, void* p_block) {
    return (uint32_t)dma_get_physical(p_dma_heap, p_block);
}

uint32_t dma_get_physical_upper(heap_t* p_dma_heap, void* p_block) {
    return (uint32_t)(dma_get_physical(p_dma_heap, p_block) >> 32);
}

int dma_heap_init(void* p_pml4, heap_t* p_dma_heap, void* p_virtual_address, size_t size) {
    if (!is_aligned((uint64_t)p_virtual_address, PAGE_SIZE))
        return 1;

    // TODO @since 13/06/2025 -- 02:06
    // we need to make an manager that only maps contigious
    if (size > PAGE_SIZE_LARGE)
        return 1;

    // the heap is just raw memory no data structures
    uint64_t heap_size = vmem_smart_alloc_pages(p_pml4, (void*)((uint64_t)p_virtual_address + PAGE_SIZE_LARGE), size);

    if (heap_size != size)
        return 2;

    // heap struct
    void* p_page = pmem_get_page();
    while (p_page && !is_aligned((uint64_t)p_page, PAGE_SIZE_LARGE)) {
        p_page = pmem_get_page();
    }

    if (p_page == nullptr)
        return 3;

    if (!pmem_try_reserve_address((void*)((uint64_t)p_page + PAGE_SIZE), (PAGE_SIZE_LARGE / PAGE_SIZE - 1)))
        return 3;

    if (!vmem_map_2mb(p_pml4, p_virtual_address, p_page))
        return 3;

    // setup heap stuct
    p_dma_heap->heap_block_array = (heap_block_t*)p_virtual_address;
    p_dma_heap->heap_block_array_size = PAGE_SIZE_LARGE / sizeof(heap_block_t);
    p_dma_heap->heap_block_count = 1;
    p_dma_heap->start_virtual_addr = (void*)((uint64_t)p_virtual_address + PAGE_SIZE_LARGE);
    p_dma_heap->size = heap_size;
    p_dma_heap->pml4 = p_pml4;

    // first block is size of entire heap, but un allocated
    p_dma_heap->heap_block_array->start_real_addr = (void*)((uint64_t)p_virtual_address + PAGE_SIZE_LARGE);
    p_dma_heap->heap_block_array->next = nullptr;
    p_dma_heap->heap_block_array->size = heap_size;
    p_dma_heap->heap_block_array->free = true;
    p_dma_heap->heap_block_array->used = true;

    return 0;
}

bool dma_heap_manager_init(dma_heap_manager_t* manager, void* pml4, void* virtual_address, size_t size) {
    if (!is_aligned(size, PAGE_SIZE_LARGE))
        return false;

    manager->heaps = std::dynamic_array<heap_t>{};
    manager->pml4 = pml4;
    manager->start_virtual_addr = virtual_address;
    manager->size = size;

    return true;
}

void set_global_dma_heap_manager(dma_heap_manager_t* manager) {
    global_dma_heap_manager = manager;
}

dma_heap_manager_t* get_global_dma_heap_manager() {
    return global_dma_heap_manager;
}

void* dma_heap_manager_get_virtual_address(dma_heap_manager_t* manager, size_t size) {
    uint64_t begin = (uint64_t)manager->start_virtual_addr;
    uint64_t end = (uint64_t)manager->start_virtual_addr + manager->size;

    while (begin + size <= end) {
        bool found = true;

        for (auto& heap : manager->heaps) {
            // take entire heap not just allocatable reagion
            uint64_t heap_begin = (uint64_t)heap.heap_block_array;
            uint64_t heap_end = (uint64_t)heap.start_virtual_addr + heap.size + PAGE_SIZE_LARGE;

            if (begin < heap_end && begin + size > heap_begin) {
                begin = heap_end;
                found = false;
                break;
            }
        }

        if (found)
            return (void*)begin;
    }

    return nullptr;
}

heap_t* dma_heap_manager_create_heap(dma_heap_manager_t* manager, size_t size) {
    void* virtual_address = dma_heap_manager_get_virtual_address(manager, size);
    if (!virtual_address)
        return nullptr;

    heap_t heap {};
    if (dma_heap_init(manager->pml4, &heap, virtual_address, size) != 0)
        return nullptr;

    manager->heaps.insert_back(heap);
    return manager->heaps.get_at(manager->heaps.length() - 1);
}

void set_global_heap(heap_t* p_heap) {
    g_heap = p_heap;
}

heap_t* get_global_heap() {
    return g_heap;
}

void* malloc(size_t size) noexcept {
    if (auto heap = get_global_heap())
        return heap_alloc(heap, size);
    
    return nullptr;
}

void free(void* ptr) noexcept {
    if (auto heap = get_global_heap())
       heap_free(heap, ptr);
}

void* malloc_aligned(size_t size, size_t align) noexcept {
    uint64_t raw = (uint64_t)malloc(size + align + sizeof(uint64_t));
    if (!raw)
        return nullptr;
    
    uint64_t* aligned = (uint64_t*)align_up(raw + sizeof(uint64_t), align);
    
    aligned[-1] = (uint64_t)raw;

    return (void*)aligned;
}

void free_aligned(void* ptr) noexcept {
    if (ptr)
        free(((void**)ptr)[-1]);
}

void* operator new(__SIZE_TYPE__ size) noexcept {
    if (auto heap = get_global_heap())
        return heap_alloc(heap, size);
    
    return nullptr;
}

void* operator new(__SIZE_TYPE__ size, void* p_ptr) noexcept {
    return p_ptr;
}

void* operator new[](__SIZE_TYPE__ size) noexcept {
    if (auto heap = get_global_heap())
        return heap_alloc(heap, size);
    return nullptr;
}

void* operator new[](__SIZE_TYPE__ size, void* ptr) noexcept {
    return ptr;
}

void operator delete(void* p_ptr) noexcept {
    if (auto heap = get_global_heap())
       heap_free(heap, p_ptr);
}

void operator delete(void* p_ptr, __SIZE_TYPE__) noexcept {
    if (auto heap = get_global_heap())
       heap_free(heap, p_ptr);
}

void operator delete[](void* ptr) noexcept {
    if (auto heap = get_global_heap())
        heap_free(heap, ptr);
}

void operator delete[](void* ptr, __SIZE_TYPE__) noexcept {
   if (auto heap = get_global_heap())
        heap_free(heap, ptr);
}