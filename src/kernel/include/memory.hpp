//==========================================
/// @file       memory.hpp
/// @brief      kernel memory mapping logic
//==========================================

#pragma once

#ifndef __MEMORY_HPP__
#define __MEMORY_HPP__

// THE SYSTEM ONLY HAD 4GB RAM !!!!
#define KERNEL_PAGE_CRITICAL_BITMAP_SIZE    0x2000 // 1024mb    0x2000 * 64 = page count
#define KERNEL_PAGE_BITMAP_SIZE             0x3000 // 1536mb    0x3000 * 64 = page count

// helper functions for page tables
#define KPAGING_GET_PE(virtual_addr, offset)    ((((uint64_t)(virtual_addr)) >> (offset)) & 0x1FF)
#define KPAGING_GET_ENTRY(table, entry)         ((uint64_t*)(((uint64_t*)(table))[((uint64_t)(entry))] & ~0xFFF))
#define KPAGING_CHECK_ENTRY(table, entry)       ((((uint64_t*)(table))[((uint64_t)(entry))]) & 1)

// page flags
#define PF_PRESENT              (1 << 0)
#define PF_READ_WRITE           (1 << 1)
#define PF_USER_SUPERVISOR      (1 << 2)
#define PF_WRITE_THROUGH        (1 << 3)
#define PF_CACHE_DISABLE        (1 << 4)
#define PF_ACCESSED             (1 << 5)
#define PF_DIRTY                (1 << 6)
#define PF_PAGE_SIZE            (1 << 7)
#define PF_GLOBAL               (1 << 8)

// heap block filter helpers
#define HEAP_MAKE_FILTER_PARAM(param) ((void*)&param)
#define HEAP_FILTERS_SIZE(array) (sizeof(array) / sizeof(block_filter_callback_t))

#define g_heap_alloc(size) heap_alloc(get_global_heap(), size)
#define g_heap_free(ptr) heap_free(get_global_heap(), ptr)

#include "common.hpp"

struct heap_block_t {
    // start of virtual address
    void* start_real_addr;

    // size of memory block
    size_t size;

    // so we can easly deallocate
    heap_block_t* next;
    
    // bitfield
    struct {
        // is the memory region used
        bool free : 1;

        // is the struct used
        // if false all other fields are invalid
        bool used : 1;
    };
} PACKED;

struct heap_t {
    heap_block_t* heap_block_array;
    size_t heap_block_array_size;
    size_t heap_block_count;
    void* start_virtual_addr;
    size_t size;
};

struct dma_heap_t {
    heap_t heap;
    void* pml4;
};

struct memory_map_entry_t {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} PACKED;

struct multiboot_info_t {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} PACKED;

struct multiboot_t {
    uint64_t magic;
    multiboot_info_t* info;
};

enum class memory_map_type_t : uint32_t {
    UNKOWN           = 0,
    USABLE           = 1,
    RESERVED         = 2,
    ACPI_RECLAIMABLE = 3,
    ACPI_NVS         = 4
};

enum class multiboot_flags_t : uint32_t {
    MEM          = (1 << 0),
    BOOT_DEVICE  = (1 << 1),
    CMDLINE      = (1 << 2),
    MODS         = (1 << 3),
    AOUT_SYMS    = (1 << 4),
    ELF_SYMS     = (1 << 5),
    MMAP         = (1 << 6),
    DRIVES       = (1 << 7),
    CONFIG_TABLE = (1 << 8),
    BOOT_LOADER  = (1 << 9),
    APM_TABLE    = (1 << 10),
    VBE          = (1 << 11)
};

typedef bool(*block_filter_callback_t)(const heap_block_t*, const void*);

/// @brief      gets a physical address in the first 1gb(unless setup otherwise)
/// @return     physical memroy address
void* vmem_get_page_critical();

/// @brief      gets a physical address after the first 1gb(unless setup otherwise)
/// @return     physical memroy address
void* vmem_get_page();

/// @brief              sets bitmap values so that those addresses
///                     can no longer be used for getting physical pages.
///                     automatically determines the bitmap
/// @param address      address to start reserving from (4k aligned)
/// @param count        amount of pages to reserve at that address
/// @return             success status
NODISCARD bool vmem_paging_reserve_at_adress(uint64_t address, size_t count = 1);

/// @brief              maps the firtst ~1gb one-one in the page table
///                     so that those values are always valid
/// @param[inout] pml4  pointer to the page table struct
void vmem_identity_map(uint64_t* pml4);

/// @brief                      maps a 4kb page to a virtual address from a phisical address
/// @param[inout] pml4          page table to use
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param[in] physical_addr    pointer to physical address to map to
/// @return                     success status
NODISCARD bool vmem_map_2kb_page(void* pml4, void* virtual_addr, void* physical_addr);

/// @brief                      maps a 2mb page to a virtual address from a phisical address
/// @param[inout] pml4          page table to use
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param[in] physical_addr    pointer to physical address to map to
/// @return                     success status
NODISCARD bool vmem_map_2mb_page(void* pml4, void* virtual_addr, void* physical_addr);

/// @brief                      allocates memory pages per 2mb if size != large page
///                             it will round up too the nearest 2mb
/// @param[inout] pml4          page table to use
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param size                 memory size to allocate
/// @return                     allocated amount of memory
NODISCARD size_t vmem_smart_alloc_pages(void* pml4, void* virtual_addr, size_t size);

NODISCARD void* vmem_virtual_to_physical(void* pml4, void* virtual_addr);
// NODISCARD void* vmem_physical_to_virtual(void* pml4, void* virtual_start, void* virtual_end, void* physical_addr);

/// @brief                      initiates virtual memory
/// @param[in] multiboot_struct pointer to the custom mb struct for memory regions
/// @param[inout] pml4          page table to use
/// @return                     success status
NODISCARD bool vmem_init(multiboot_t* multiboot_struct, void* pml4);

/// @brief      filters for block finding / filtering (undocumented filters)
namespace heap_block_filters {

bool donor_block_filter(const heap_block_t* block, const void* param);
bool unused_block_filter(const heap_block_t* block, const void* param);
bool last_block_filter(const heap_block_t* block, const void* param);

} // namespace heap_block_filters

/// @brief                      finds blockes in the heap based on the filter given
///                             filter 1 == block 1 ... etc
///                             1 filter per block
/// @param[in] heap             heap in which to search
/// @param[in] param            extra parameter to give to the filter functions
/// @param[in] bfc_array        array to filter functions
/// @param bfc_array_size       size of the filter function array
/// @param[out] block_array     array which gets filled with the (found) blocks
/// @param block_array_size     max size of the block array
/// @return                     number of found block or -1 if bfc_array_size != block_array_size
/// @remarks                    this function cant use the vector since it requires
///                             memory to be setup already :)
int heap_filter_blocks(heap_t* heap, void* param, block_filter_callback_t bfc_array[], size_t bfc_array_size, heap_block_t* block_array[], size_t block_array_size);

/// @brief                      initiates a heap
/// @param[inout] heap          heap to initiate
/// @param[inout] pml4          page table to use
/// @param[in] virtual_address  virtual address to start the heap from
/// @param size                 starting size of the heap
/// @return                     success status
NODISCARD bool heap_init(heap_t* heap, void* pml4, void* virtual_address, size_t size);

/// @brief              expands the heap ontop of current heap
///                     make sure the virtual address above is not yet reserved
/// @param[inout] heap  heap to expand
/// @param[inout] pml4  page table to use
/// @param size         size to expand the heap by
/// @return             success status
NODISCARD bool heap_expand(heap_t* heap, void* pml4, size_t size);

/// @brief              allocates memory from a heap
/// @param[in] heap     heap to allocate from
/// @param size         memory block size
/// @return             ptr to memory or nullptr if no more memory is available
void* heap_alloc(heap_t* heap, size_t size);

/// @brief              deallocates memory from a heap
/// @param[in] heap     heap to free from
/// @param[in] ptr      pointer to memory
void heap_free(heap_t* heap, void* ptr);

/// @brief                          checks if the multiboot magic was valid
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         true if mb magic was valid or nullptr if there is no block
bool mb_has_valid_magic(multiboot_t* multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to mb struct
/// @return                         pointer to first entry
memory_map_entry_t* mb_get_first_entry(multiboot_t* multiboot_struct);

/// @brief                          helper for looping over mb entries
/// @param[in] multiboot_struct     pointer to the mb struct
/// @param[in] prev                 pointer to last mme
/// @return                         pointer to next entry or nullptr if there are no more blocks
memory_map_entry_t* mb_get_next_entry(multiboot_t* multiboot_struct, memory_map_entry_t* prev);

/// @brief                      allocates a aligned mem block
/// @param[inout] dma_heap      pointer to the dma_heap_t struct
/// @return                     ptr to the block
void* dma_heap_alloc(dma_heap_t* dma_heap, size_t size, uint64_t align);

/// @brief                      frees the given block
/// @param[inout] dma_heap      ptr to the dma_heap_t struct where the block is located
/// @param[in] block            ptr to the block to free
void dma_heap_free(dma_heap_t* dma_heap, void* block);

/// @brief                  converts a virtual addr in the dma heap to a physical addr
/// @param[inout] dma_heap  heap that contains the block
/// @param[in] block        ptr to the block / the virtual addr
/// @return                 physical addr
uint64_t dma_get_physical(dma_heap_t* dma_heap, void* block);

/// @brief                  converts a virtual addr in the dma heap to a physical addr
///                         lower 32 bits
/// @param[inout] dma_heap  heap that contains the block
/// @param[in] block        ptr to the block / the virtual addr
/// @return                 physical addr lower 32 bits
uint32_t dma_get_physical_lower(dma_heap_t* dma_heap, void* block);

/// @brief                  converts a virtual addr in the dma heap to a physical addr
///                         upper 32 bits
/// @param[inout] dma_heap  heap that contains the block
/// @param[in] block        ptr to the block / the virtual addr
/// @return                 physical addr upper 32 bits
uint32_t dma_get_physical_upper(dma_heap_t* dma_heap, void* block);

/// @brief                          initializes a dma heap
/// @param[inout] pml4              ptr to the page table to use
/// @param[inout] dma_heap          ptr to the dma_heap_t
/// @param[in] virtual_address      
/// @param size                     
/// @return                         0 success, 1 incorrect alignment, 2 heap size failed, 3 heap init failed
/// @remarks                        technically memory leaks since it never frees
///                                 the memory used for the entire dma heap
NODISCARD int dma_heap_init(void* pml4, dma_heap_t* dma_heap, void* virtual_address, size_t size);

/// @brief              sets the global heap to the given one
/// @param[in] heap     pointer to heap that should be global
void set_global_heap(heap_t* heap);

/// @brief      gets the global heap if set
/// @return     pointer to global heap
heap_t* get_global_heap();

void* operator new(size_t size) noexcept;
// void* operator new[](size_t size);

void* operator new(size_t size, void* ptr) noexcept;
// void* operator new[](size_t size, void* ptr);

void operator delete(void* ptr) noexcept;
// void operator delete[](void* ptr);

void operator delete(void* ptr, size_t size) noexcept;
// void operator delete[](void* ptr, size_t size);

#endif // __MEMORY_HPP__