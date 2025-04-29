#include "memory.hpp"

// variable placed at the end of the kernel
extern "C" uint64_t __lnk_end_kernel;

static uint64_t kernel_page_critical_bitmap[KERNEL_PAGE_CRITICAL_BITMAP_SIZE] {};
static uint64_t kernel_page_bitmap[KERNEL_PAGE_BITMAP_SIZE] {};

static heap_t* g_heap = nullptr;

void __flush_tlb(uint64_t* virtual_address) { asm volatile("invlpg (%0)" : : "r"(virtual_address) : "memory"); }
void __set_pml4(void* ptr) { asm volatile("mov %0, %%cr3" : : "r"(ptr) : "memory"); }
void* __get_pml4() { void* pml4; asm volatile("mov %%cr3, %0" : "=r"(pml4) : : "memory"); return pml4; }

void* vmem_get_page_critical() {
    // slow ahh
    for (size_t i = 1; i < KERNEL_PAGE_CRITICAL_BITMAP_SIZE; i++) {
        if (!bitmap_get(kernel_page_critical_bitmap, i)) {
            bitmap_set(kernel_page_critical_bitmap, i, true);
            return (void*)(i * PAGE_SIZE);
        }
    }

    return nullptr;
}

void* vmem_get_page() {
    // slow ahh
    for (size_t i = 1; i < KERNEL_PAGE_BITMAP_SIZE; i++) {
        if (!bitmap_get(kernel_page_bitmap, i)) {
            bitmap_set(kernel_page_bitmap, i, true);
            return (void*)((i + KERNEL_PAGE_CRITICAL_BITMAP_SIZE) * PAGE_SIZE);
        }
    }

    return nullptr;
}

bool vmem_paging_reserve_at_adress(uint64_t address, size_t count) {
    if (!mem_is_aligned(address, PAGE_SIZE))
        return false;

    constexpr uint64_t end_critical_memory_space = KERNEL_PAGE_CRITICAL_BITMAP_SIZE * PAGE_SIZE;
    constexpr uint64_t end_memory_space = end_critical_memory_space + KERNEL_PAGE_BITMAP_SIZE * PAGE_SIZE;

    if (address + (PAGE_SIZE * count) < end_critical_memory_space) {
        size_t page_start_index = address / PAGE_SIZE;
        size_t current_count = 0;
        for (size_t i = page_start_index; i < KERNEL_PAGE_CRITICAL_BITMAP_SIZE; i++) {
            !bitmap_get(kernel_page_critical_bitmap, i)
                ? current_count++
                : current_count = 0;

            // found section so we reserve it
            if (current_count == count) {
                for (; current_count > 0; current_count--)
                    bitmap_set(kernel_page_critical_bitmap, i - current_count, true);
                return true;
            }
        }

        return false;
    }

    if (address + (PAGE_SIZE * count) < end_memory_space) {
        size_t page_start_index = (address - end_critical_memory_space) / PAGE_SIZE;
        if (page_start_index < 0)
            return false;

        size_t current_count = 0;
        for (size_t i = page_start_index; i < KERNEL_PAGE_BITMAP_SIZE; i++) {
            !bitmap_get(kernel_page_bitmap, i)
                ? current_count++
                : current_count = 0;

            // found section so we reserve it
            if (current_count == count) {
                for (; current_count > 0; current_count--)
                    bitmap_set(kernel_page_bitmap, i - current_count, true);
                return true;
            }
        }

        return false;
    }

    return false;
}

void vmem_identity_map(uint64_t* pml4) {
    if (!KPAGING_CHECK_ENTRY(pml4, 0)) {
        auto page = vmem_get_page_critical();
        pml4[0] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdpt = KPAGING_GET_ENTRY(pml4, 0);
    if (!KPAGING_CHECK_ENTRY(pdpt, 0)) {
        auto page = vmem_get_page_critical();
        pdpt[0] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    // identity map the first 1 GB
    auto pdt = KPAGING_GET_ENTRY(pdpt, 0);
    for (uint64_t pde = 0; pde < 512; pde++) {
        uint64_t physical_addr = (uint64_t)pde * PAGE_SIZE_LARGE;
        pdt[pde] = (physical_addr & ~0x1FFFFF) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE;
        __flush_tlb((uint64_t*)physical_addr);
    }
}

bool vmem_map_2kb_page(void* pml4, void* virtual_addr, void* physical_addr) {
    if (!mem_is_aligned((uint64_t)virtual_addr, PAGE_SIZE) ||
        !mem_is_aligned((uint64_t)physical_addr, PAGE_SIZE)) {
        return false;
    }

    const uint64_t pml4e =    KPAGING_GET_PE(virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(virtual_addr, 21);
    const uint64_t pte =      KPAGING_GET_PE(virtual_addr, 12);

    if (!KPAGING_CHECK_ENTRY(pml4, pml4e)) {
        auto page = vmem_get_page_critical();
        ((uint64_t*)pml4)[pml4e] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdpt = KPAGING_GET_ENTRY(pml4, pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe)) {
        auto page = vmem_get_page_critical();
        pdpt[pdpe] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdt = KPAGING_GET_ENTRY(pdpt, pdpe);
    if (!KPAGING_CHECK_ENTRY(pdt, pde)) {
        auto page = vmem_get_page_critical();
        pdt[pde] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto ptt = KPAGING_GET_ENTRY(pdt, pde);
    ptt[pte] = ((uint64_t)physical_addr & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;
    __flush_tlb((uint64_t*)virtual_addr);

    return true;
}

bool vmem_map_2mb_page(void* pml4, void* virtual_addr, void* physical_addr) {
    if (!mem_is_aligned((uint64_t)virtual_addr, PAGE_SIZE_LARGE) ||
        !mem_is_aligned((uint64_t)physical_addr, PAGE_SIZE_LARGE)) {
        return false;
    }

    const uint64_t pml4e =    KPAGING_GET_PE(virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(virtual_addr, 21);

    if (!KPAGING_CHECK_ENTRY(pml4, pml4e)) {
        auto page = vmem_get_page_critical();
        ((uint64_t*)pml4)[pml4e] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    uint64_t* pdpt = KPAGING_GET_ENTRY(pml4, pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe)) {
        auto page = vmem_get_page_critical();
        pdpt[pdpe] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    uint64_t* pdt = KPAGING_GET_ENTRY(pdpt, pdpe);
    pdt[pde] = ((uint64_t)physical_addr & ~0x1FFFFF) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE;
    __flush_tlb((uint64_t*)virtual_addr);

    return true;
}

size_t vmem_smart_alloc_pages(void* pml4, void* virtual_addr, size_t size) {
    size_t allocated = 0;
    uint64_t current_virtual_address = (uint64_t)virtual_addr;

    while (allocated < size) {
        uint64_t target_physical_address = (uint64_t)vmem_get_page();

        // force 2mb memory alignment
        if (!mem_is_aligned(target_physical_address, PAGE_SIZE_LARGE))
            continue;

        // reserve the remaining pages to complete 2MB
        if (!vmem_paging_reserve_at_adress(target_physical_address + PAGE_SIZE, (PAGE_SIZE_LARGE / PAGE_SIZE - 1)))
            return 0;

        // map the address
        if (!vmem_map_2mb_page(pml4, (void*)current_virtual_address, (void*)target_physical_address))
            return false;

        // update counters
        allocated += PAGE_SIZE_LARGE;
        current_virtual_address += PAGE_SIZE_LARGE;
    }

    return allocated;
}

bool vmem_init(multiboot_t* multiboot_struct, void* pml4) {
    // zero page
    memzero(0, PAGE_SIZE);

    const uint64_t aligned_kernel_end_addr = mem_align_up((uint64_t)&__lnk_end_kernel, PAGE_SIZE_LARGE);
    const uint64_t kernel_page_count = aligned_kernel_end_addr / PAGE_SIZE;
    if (!vmem_paging_reserve_at_adress(0, kernel_page_count))
        return false;

    for (auto mm_entry = mb_get_first_entry(multiboot_struct); mm_entry; mm_entry = mb_get_next_entry(multiboot_struct, mm_entry)) {
        // reserve physical pages for reserved memory
        if (mm_entry->type != (uint32_t)memory_map_type_t::USABLE) {
            if (mm_entry->addr + mm_entry->len > (KERNEL_PAGE_BITMAP_SIZE + KERNEL_PAGE_CRITICAL_BITMAP_SIZE) * PAGE_SIZE)
                continue;

            // reserve as much as possible
            for (size_t i = 0; i < mm_entry->len; i += PAGE_SIZE) {
                if (!vmem_paging_reserve_at_adress(mem_align_down(mm_entry->addr, PAGE_SIZE) + i))
                    return false;
            }
        }
    }

    // first 1G is identity mapped
    vmem_identity_map((uint64_t*)pml4);
    __set_pml4(pml4);

    return true;
}

bool heap_block_filters::donor_block_filter(const heap_block_t* block, const void* param) {
    return block->used && block->free && block->size >= *(size_t*)param;
}

bool heap_block_filters::unused_block_filter(const heap_block_t* block, const void* param) {
    return !block->used;
}

bool heap_block_filters::last_block_filter(const heap_block_t* block, const void* param) {
    return block->used && !block->next;
}

int heap_filter_blocks(heap_t* heap, void* param, block_filter_callback_t bfc_array[], size_t bfc_array_size, heap_block_t* block_array[], size_t block_array_size) {
    if (bfc_array_size != block_array_size)
        return -1;

    size_t found_size = 0;
    for (size_t i = 0; i < heap->heap_block_array_size; i++) {
        if (found_size == block_array_size)
            break;
        
        heap_block_t* current_block = &heap->heap_block_array[i];

        for (size_t k = 0; k < bfc_array_size; k++) {
            heap_block_t** stored_block = &block_array[k];
            block_filter_callback_t filter_func = bfc_array[k];

            if (*stored_block == nullptr) {
                if (filter_func(current_block, param)) {
                    *stored_block = current_block;
                    found_size++;
                    break;
                }
            }
        }
    }

    return found_size;
}

bool heap_init(heap_t* heap, void* pml4, void* virtual_address, size_t size) {
    // the heap is just raw memory no data structures
    uint64_t heap_size = vmem_smart_alloc_pages(pml4, virtual_address, size);

    // heap struct (identity mapped memory)
    void* p_page = vmem_get_page_critical();
    if (!vmem_paging_reserve_at_adress((uint64_t)p_page + PAGE_SIZE, (PAGE_SIZE_LARGE / PAGE_SIZE - 1)))
        return false;

    if (!vmem_map_2mb_page(pml4, p_page, p_page))
        return false;

    // setup heap stuct
    heap->heap_block_array = (heap_block_t*)p_page;
    heap->heap_block_array_size = PAGE_SIZE_LARGE / sizeof(heap_block_t);
    heap->heap_block_count = 1;
    heap->start_virtual_addr = virtual_address;
    heap->size = heap_size;

    // first block is size of entire heap, but un allocated
    heap->heap_block_array->start_real_addr = virtual_address;
    heap->heap_block_array->next = nullptr;
    heap->heap_block_array->size = heap_size;
    heap->heap_block_array->free = true;
    heap->heap_block_array->used = true;

    return true;
}

bool heap_expand(heap_t* heap, void* pml4, size_t size) {
    block_filter_callback_t filters[] = {
        heap_block_filters::last_block_filter,
        heap_block_filters::unused_block_filter
    };
    heap_block_t* blocks[HEAP_FILTERS_SIZE(filters)] = {};
    
    int result = heap_filter_blocks(heap, HEAP_MAKE_FILTER_PARAM(size), filters, HEAP_FILTERS_SIZE(filters), blocks, HEAP_FILTERS_SIZE(filters));

    if (result != HEAP_FILTERS_SIZE(filters))
        return false;

    void* heap_virtual_end = (void*)((uint64_t)heap->start_virtual_addr + heap->size);
    const auto new_heap_block_size = vmem_smart_alloc_pages(pml4, heap_virtual_end, size);

    heap->size += new_heap_block_size;

    heap_block_t* new_block = blocks[1];
    new_block->used = true;
    new_block->free = true;
    new_block->size = new_heap_block_size;
    new_block->start_real_addr = heap_virtual_end;
    
    heap_block_t* last_block = blocks[0];
    last_block->next = new_block;

    return true;
}

void* heap_alloc(heap_t* heap, size_t size) {
    // not needed
    // if (size > PAGE_SIZE_LARGE)
    //     return nullptr;

    block_filter_callback_t filters[] = {
        heap_block_filters::donor_block_filter,
        heap_block_filters::unused_block_filter
    };
    heap_block_t* blocks[HEAP_FILTERS_SIZE(filters)] = {};
    
    int result = heap_filter_blocks(heap, HEAP_MAKE_FILTER_PARAM(size), filters, HEAP_FILTERS_SIZE(filters), blocks, HEAP_FILTERS_SIZE(filters));

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
        // kinda sketch to just get a "random" pml4 but whtv
        if (!heap_expand(heap, __get_pml4(), PAGE_SIZE_LARGE))
            return nullptr;

        return heap_alloc(heap, size);
    }

    donor_block->size -= size;

    unused_block->free = true;
    unused_block->used = true;
    unused_block->size = size;
    unused_block->start_real_addr = (void*)((uint64_t)donor_block->start_real_addr + donor_block->size);

    unused_block->next = donor_block->next;
    donor_block->next = unused_block;

    return unused_block->start_real_addr;
}

void heap_free(heap_t* heap, void* ptr) {
    for (heap_block_t* current_block = heap->heap_block_array; current_block; current_block = current_block->next) {
        // find the correct block & free it
        if (current_block->start_real_addr == ptr) {
            current_block->free = true;
            break;
        }
    }

    for (heap_block_t* current_block = heap->heap_block_array; current_block; current_block = current_block->next) {
        heap_block_t* next_block = current_block->next;
        if (current_block->free && next_block && next_block->free) {
            
            current_block->size += next_block->size;
            current_block->next = next_block->next;

            memzero((void*)next_block, sizeof(heap_block_t));
        }
    }
}

bool mb_has_valid_magic(multiboot_t* multiboot_struct) {
    return multiboot_struct->magic == 0x2BADB002;
}

memory_map_entry_t* mb_get_first_entry(multiboot_t* multiboot_struct) {
    const auto& mbi = multiboot_struct->info;

    if (!(mbi->flags & (uint32_t)multiboot_flags_t::MMAP))
        return nullptr;

    auto memory_map_entry = (memory_map_entry_t*)(uint64_t)mbi->mmap_addr;

    if ((uint64_t)memory_map_entry < (uint64_t)(mbi->mmap_addr + mbi->mmap_length))
        return memory_map_entry;

    return nullptr;
}

memory_map_entry_t* mb_get_next_entry(multiboot_t* multiboot_struct, memory_map_entry_t* prev) {
    const auto& mbi = multiboot_struct->info;

    auto memory_map_entry = (memory_map_entry_t*)((uint64_t)prev + prev->size + sizeof(prev->size));

    if ((uint64_t)memory_map_entry < (uint64_t)(mbi->mmap_addr + mbi->mmap_length))
        return memory_map_entry;
    
    return nullptr;
}

void set_global_heap(heap_t* heap) {
    g_heap = heap;
}

heap_t* get_global_heap() {
    return g_heap;
}

void* operator new(size_t size) noexcept {
    if (get_global_heap() == nullptr)
        return nullptr;
    return heap_alloc(get_global_heap(), size);
}

void* operator new(size_t size, void* ptr) noexcept {
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (get_global_heap() != nullptr)
       heap_free(get_global_heap(), ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    if (get_global_heap() != nullptr)
        heap_free(get_global_heap(), ptr);
}