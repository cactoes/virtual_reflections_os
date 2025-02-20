#define __KERNEL_MEMORY_FULL__

#include "critical/memory.hpp"
#include "bitmap.hpp"

extern "C" uint64_t end_kernel;
extern "C" uint64_t multiboot_info;

void __flush_tlb(uint64_t* virtual_address) { asm volatile("invlpg (%0)" : : "r"(virtual_address) : "memory"); }
void __set_pml4t(uint64_t* pp) { asm volatile("mov %0, %%cr3" : : "r"(pp) : "memory"); }

kernel::memory::heap_t*     g_kernel_heap = nullptr;
uint64_t*                   g_kernel_pml4t = nullptr;

bitmap<KMEM_PAGE_TABLE_SIZE_KERNEL>     g_kernel_bitmap {};
bitmap<KMEM_PAGE_TABLE_SIZE_USER>       g_user_bitmap {};

void* memset(void* dest, uint8_t val, size_t size) {
    if (size == 0)
        return dest;

    uint8_t* p_dest = (uint8_t*)dest;

    for (size_t i = 0; i < size; i++)
        p_dest[i] = val;

    return dest;
}

void* memzero(void* dest, size_t size) {
    return memset(dest, 0, size);
}

void* memcpy(void* dest, const void* src, size_t size) {
    if (size == 0)
        return dest;

    uint8_t* p_dest = (uint8_t*)dest;
    const uint8_t* p_src = (uint8_t*)src;

    for (size_t i = 0; i < size; i++)
        p_dest[i] = p_src[i];

    return dest;
}

bool kernel::memory::check_alignment(uint64_t addr, uint64_t align) {
    return (addr & (align - 1)) == 0;
}

uint64_t kernel::memory::align_up_to(uint64_t addr, uint64_t align) {
    return (addr + (align - 1)) & ~(align - 1);
}

void* kernel::memory::paging::get_page() {
    if (g_user_bitmap.get_top() < KMEM_PAGE_TABLE_SIZE_USER)
        // user is after kernel
        return (void*)((KMEM_PAGE_TABLE_SIZE_KERNEL + g_user_bitmap.set_next_free()) * PAGE_SIZE);

    kernel_fatal(KFATAL_PAGE_TABLE_USER_LIMIT_REACHED);
    return 0;
}

void* kernel::memory::paging::get_page_kernel() {
    if (g_kernel_bitmap.get_top() < KMEM_PAGE_TABLE_SIZE_KERNEL)
        return (void*)(g_kernel_bitmap.set_next_free() * PAGE_SIZE);

    kernel_fatal(KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED);
    return 0;
}

void kernel::memory::paging::page_reserve(size_t count) {
    for (size_t i = 0; i < count; i++)
        get_page();
}

void kernel::memory::paging::page_reserve_kernel(size_t count) {
    for (size_t i = 0; i < count; i++)
        get_page_kernel();
}

void kernel::memory::paging::page_reserve_at_index(uint64_t address) {
    if (!check_alignment(address, PAGE_SIZE))
        kernel_fatal(KFATAL_MEMORY_ALIGNMENT_INCORRECT);

    constexpr uint64_t end_kernel_memory_space = KMEM_PAGE_TABLE_SIZE_KERNEL * PAGE_SIZE;
    constexpr uint64_t end_user_memory_space = end_kernel_memory_space + KMEM_PAGE_TABLE_SIZE_USER * PAGE_SIZE;

    if (address + PAGE_SIZE < end_kernel_memory_space) {
        g_kernel_bitmap.set((size_t)(address / PAGE_SIZE), true);
    } else if (address + PAGE_SIZE < end_user_memory_space) {
        g_user_bitmap.set((size_t)((address - end_kernel_memory_space) / PAGE_SIZE), true);
    } else {
        kernel_fatal(KFATAL_MEMORY_OUT_OF_BOUNDS);
    }
}

void kernel::memory::paging::map_small_page(uint64_t* pml4t, void* virtual_addr, void* physical_addr) {
    if (!check_alignment((uint64_t)virtual_addr, PAGE_SIZE) ||
        !check_alignment((uint64_t)physical_addr, PAGE_SIZE)) {

        kernel_fatal(KFATAL_MEMORY_ALIGNMENT_INCORRECT);
    }

    const uint64_t pml4e =    KPAGING_GET_PE(virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(virtual_addr, 21);
    const uint64_t pte =      KPAGING_GET_PE(virtual_addr, 12);

    if (!KPAGING_CHECK_ENTRY(pml4t, pml4e)) {
        auto page = get_page_kernel();
        pml4t[pml4e] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdpt = KPAGING_GET_ENTRY(pml4t, pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe)) {
        auto page = get_page_kernel();
        pdpt[pdpe] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdt = KPAGING_GET_ENTRY(pdpt, pdpe);
    if (!KPAGING_CHECK_ENTRY(pdt, pde)) {
        auto page = get_page_kernel();
        pdt[pde] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto ptt = KPAGING_GET_ENTRY(pdt, pde);
    ptt[pte] = ((uint64_t)physical_addr & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;
}

void kernel::memory::paging::map_large_page(uint64_t* pml4t, void* virtual_addr, void* physical_addr) {
    if (!check_alignment((uint64_t)virtual_addr, PAGE_SIZE_LARGE) ||
        !check_alignment((uint64_t)physical_addr, PAGE_SIZE_LARGE)) {

        kernel_fatal(KFATAL_MEMORY_ALIGNMENT_INCORRECT);
    }

    const uint64_t pml4e =    KPAGING_GET_PE(virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(virtual_addr, 21);

    if (!KPAGING_CHECK_ENTRY(pml4t, pml4e)) {
        auto page = get_page_kernel();
        pml4t[pml4e] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    uint64_t* pdpt = KPAGING_GET_ENTRY(pml4t, pml4e);
    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe)) {
        auto page = get_page_kernel(); // allocs a page in unmapped memory
        pdpt[pdpe] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    uint64_t* pdt = KPAGING_GET_ENTRY(pdpt, pdpe);
    pdt[pde] = ((uint64_t)physical_addr & ~0x1FFFFF) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE;
    __flush_tlb((uint64_t*)virtual_addr);
}

uint64_t kernel::memory::paging::virtual_to_physical(uint64_t* pml4t, void* virtual_addr) {
    const uint64_t pml4e =    KPAGING_GET_PE(virtual_addr, 39);
    const uint64_t pdpe =     KPAGING_GET_PE(virtual_addr, 30);
    const uint64_t pde =      KPAGING_GET_PE(virtual_addr, 21);

    if (!KPAGING_CHECK_ENTRY(pml4t, pml4e))
        return 0;
    
    uint64_t* pdpt = KPAGING_GET_ENTRY(pml4t, pml4e);

    if (!KPAGING_CHECK_ENTRY(pdpt, pdpe))
        return 0;

    uint64_t* pdt = KPAGING_GET_ENTRY(pdpt, pdpe);

    if (!KPAGING_CHECK_ENTRY(pdt, pde))
        return 0;

    if (!(pdt[pde] & PF_PAGE_SIZE))
        kernel_fatal(KFATAL_PAGE_TABLE_INCORRECT_SIZE);

    const uint64_t page_offset = (uint64_t)virtual_addr & (PAGE_SIZE_LARGE - 1);
    return (pdt[pde] & ~(PAGE_SIZE_LARGE - 1)) + page_offset;
}

void __identity_map(uint64_t* pml4t) {
    if (!KPAGING_CHECK_ENTRY(pml4t, 0)) {
        auto page = kernel::memory::paging::get_page_kernel();
        pml4t[0] = (uint64_t)page | PF_PRESENT | PF_READ_WRITE;
        memzero(page, PAGE_SIZE);
    }

    auto pdpt = KPAGING_GET_ENTRY(pml4t, 0);
    if (!KPAGING_CHECK_ENTRY(pdpt, 0)) {
        auto page = kernel::memory::paging::get_page_kernel();
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

typedef struct __MB_MMAP_ENTRY {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

typedef struct __MB_INFO {
    uint32_t flags;

    // Memory info
    uint32_t mem_lower;
    uint32_t mem_upper;

    // Boot device
    uint32_t boot_device;

    // Command line
    uint32_t cmdline;

    // Modules
    uint32_t mods_count;
    uint32_t mods_addr;

    // ELF section headers
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;

    // Memory map
    uint32_t mmap_length;
    uint32_t mmap_addr;

    // Drive info
    uint32_t drives_length;
    uint32_t drives_addr;

    // Configuration table
    uint32_t config_table;
    uint32_t boot_loader_name;

    // APM table
    uint32_t apm_table;

    // Video information
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} __attribute__((packed)) multiboot_info_t;

enum mmap_type_t {
    MMAPT_UNKOWN = 0,
    MMAPT_USABLE,
    MMAPT_RESERVED,
    MMAPT_ACPI_RECLAIMABLE,
    MMAPT_ACPI_NVS
};

void kernel::memory::vmem::init(heap_t* heap, uint64_t* pml4t, void* multiboot_info) {
    // zero page
    memzero(0, PAGE_SIZE);

    // keep kernel safe
    uint64_t end_kernel_aligned = align_up_to((size_t)&end_kernel, PAGE_SIZE_LARGE);
    size_t kernel_page_count = (end_kernel_aligned / PAGE_SIZE);
    paging::page_reserve_kernel(kernel_page_count);

    // map non free pages
    auto mbi = (multiboot_info_t*)multiboot_info;
    if (mbi->flags & (1 << 6)) {
        multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*)(uint64_t)mbi->mmap_addr;
        uint32_t mmap_length = mbi->mmap_length;

        while ((uint64_t)mmap < mbi->mmap_addr + mmap_length) {
            // check if space is reserved
            if (mmap->type != MMAPT_USABLE) {
                // if so reserve it in our bitmap
                for (uint64_t i = 0; i < mmap->len / PAGE_SIZE; i++) {
                    if (mmap->addr + mmap->len < (KMEM_PAGE_TABLE_SIZE_KERNEL + KMEM_PAGE_TABLE_SIZE_USER) * PAGE_SIZE)
                        paging::page_reserve_at_index(mmap->addr + i * PAGE_SIZE);
                }
            }

            // TODO: reclaime MMAPT_ACPI_RECLAIMABLE
            mmap = (multiboot_mmap_entry_t*)((uint64_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }

    // setup pml4
    __identity_map(pml4t);
    __set_pml4t(pml4t);

    g_kernel_heap = heap;
    g_kernel_pml4t = pml4t;

    init_kernel_heap(heap, pml4t, (void*)KHEAP_VIRTUAL_START, KHEAP_START_SIZE);
}

size_t smart_alloc_pages(uint64_t* pml4t, const void* start_adress, size_t size) {
    size_t allocated = 0;
    uint64_t current_virtual_address = (uint64_t)start_adress;

    while (allocated < size) {
        uint64_t target_physical_address = (uint64_t)kernel::memory::paging::get_page();

        // force 2mb memory alignment
        if (!kernel::memory::check_alignment(target_physical_address, PAGE_SIZE_LARGE))
            continue;

        // reserve the remaining pages
        // FIXME: rework this logic with new page bitmap ...
        kernel::memory::paging::page_reserve(PAGE_SIZE_LARGE / PAGE_SIZE - 1);

        // map the address
        kernel::memory::paging::map_large_page(pml4t, (void*)current_virtual_address, (void*)target_physical_address);

        // update counter
        allocated += PAGE_SIZE_LARGE;

        // update virtual address
        current_virtual_address += PAGE_SIZE_LARGE;
    }

    return allocated;
}

void kernel::memory::vmem::init_kernel_heap(heap_t* heap, uint64_t* pml4t, void* start_adress, size_t size) {
    memzero(heap, sizeof(heap_t));

    uint64_t heap_size = smart_alloc_pages(pml4t, start_adress, size);

    heap->start = start_adress;
    heap->size = heap_size;

    heap->first_block = (heap_block_t*)heap->start;
    memzero(heap->first_block, sizeof(heap_block_t));

    heap->first_block->size = heap->size - sizeof(heap_block_t);
    heap->first_block->free = true;
    heap->first_block->next = nullptr;
    heap->first_block->prev = nullptr;
}

void kernel::memory::vmem::kernel_heap_expand(heap_t* heap, uint64_t* pml4t, uint64_t size) {
    void* heap_end = (void*)((uint64_t)heap->start + heap->size);
    uint64_t new_block_size = smart_alloc_pages(pml4t, heap_end, size + sizeof(heap_block_t) + 1);
    heap->size += new_block_size;

    heap_block_t* last_heap_block = heap->first_block;
    while (last_heap_block->next)
        last_heap_block = last_heap_block->next;

    heap_block_t* new_block = (heap_block_t*)heap_end;
    memzero(new_block, sizeof(heap_block_t));

    new_block->size = new_block_size - sizeof(heap_block_t);
    new_block->free = true;
    new_block->next = nullptr;
    new_block->prev = last_heap_block;
    last_heap_block->next = new_block;
}

void* kernel::memory::vmem::kalloc(size_t size) {
    // dont allow blocks bigger than 2mb
    if (size > PAGE_SIZE_LARGE)
        return nullptr;

    heap_block_t* current_block = g_kernel_heap->first_block;

    while (current_block) {
        if (current_block->free) {
            if (current_block->size > size + sizeof(heap_block_t)) {
                heap_block_t* new_block_at_end = (heap_block_t*)((uint64_t)current_block + sizeof(heap_block_t) + size);
                memzero(new_block_at_end, sizeof(heap_block_t));

                new_block_at_end->size = current_block->size - size - sizeof(heap_block_t);
                new_block_at_end->free = true;
                new_block_at_end->prev = current_block;
                new_block_at_end->next = current_block->next;

                current_block->size = size;
                current_block->free = false;
                current_block->next = new_block_at_end;

                return (void*)((uint64_t)current_block + sizeof(heap_block_t));
            }
        }

        current_block = current_block->next;
    }

    kernel_heap_expand(g_kernel_heap, g_kernel_pml4t, size + sizeof(heap_block_t) + 1);
    return kalloc(size);
}

void kernel::memory::vmem::kfree(void* ptr) {
    if (ptr == nullptr)
        return;

    heap_block_t* heap_block_ptr = (heap_block_t*)((uint64_t)ptr - sizeof(heap_block_t));
    heap_block_ptr->free = true;

    if (heap_block_ptr->next && heap_block_ptr->next->free) {
        heap_block_ptr->size += sizeof(heap_block_t) + heap_block_ptr->next->size;
        heap_block_ptr->next = heap_block_ptr->next->next;
    }

    heap_block_t* current_block = g_kernel_heap->first_block;
    while (current_block && current_block->next != heap_block_ptr)
        current_block = current_block->next;

    if (current_block && current_block->free) {
        current_block->size += sizeof(heap_block_t) + heap_block_ptr->size;
        current_block->next = heap_block_ptr->next;
    }
}

void* operator new(size_t size) {
    // if (g_kernel_heap == nullptr)
    //     return nullptr;
    return kernel::memory::vmem::kalloc(size);
}

void* operator new(size_t size, void* ptr) {
    return ptr;
}

void operator delete(void* ptr) {
    if (g_kernel_heap != nullptr)
        kernel::memory::vmem::kfree(ptr);
}

void operator delete(void* ptr, size_t) {
    if (g_kernel_heap != nullptr)
        kernel::memory::vmem::kfree(ptr);
}