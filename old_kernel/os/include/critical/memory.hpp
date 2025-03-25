//==========================================
/// @file       memory.hpp
/// @brief      kernel memory mapping logic
//==========================================

#pragma once

#ifndef __MEMORY_HPP___
#define __MEMORY_HPP__

#include "common.hpp"
#include "kernel.hpp"

// max page entries
#define PAGE_ENTRIES        0x200

// page sizes
#define PAGE_SIZE           0x1000
#define PAGE_SIZE_LARGE     0x200000
#define PAGE_SIZE_HUGE      0x40000000

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

// kernel memory page counts
#define KMEM_PAGE_TABLE_SIZE_KERNEL 0x20000     // 512 MB max size memory
#define KMEM_PAGE_TABLE_SIZE_USER   0x40000     // 1 GB max size memory

// kernel heap memory sizes & virtual addresses
#define KHEAP_VIRTUAL_START         0x40000000          // PDPE == 1
#define KHEAP_START_SIZE            0x100000 * 32       // 32 MB starting

// helper functions for page tables
#define KPAGING_GET_PE(virtual_addr, offset)    ((((uint64_t)(virtual_addr)) >> (offset)) & 0x1FF)
#define KPAGING_GET_ENTRY(table, entry)         ((uint64_t*)(((uint64_t*)(table))[((uint64_t)(entry))] & ~0xFFF))
#define KPAGING_CHECK_ENTRY(table, entry)       ((((uint64_t*)(table))[((uint64_t)(entry))]) & 1)

/// @brief              sets a section of memory to a value
/// @param[in] dest     pointer to target
/// @param val          value to set target to
/// @param size         total bytes to set
/// @return             pointer to dest
void*
memset(
    void* dest,
    uint8_t val,
    size_t size);

/// @brief              zero's out e memory block
/// @param[in] dest     pointer to target
/// @param size         total bytes to set
/// @return             pointer to dest
void*
memzero(
    void* dest,
    size_t size);

/// @brief              copy's a block of memory to the target
/// @param[in] dest     pointer to target
/// @param[in] src      pointer to the source
/// @param size         total bytes to copy
/// @return             pointer to dest
void*
memcpy(
    void* dest,
    const void* src,
    size_t size);

/// @brief ifdef for full memory implementation
///        which is not required for 90% of memory imports
#ifdef __KERNEL_MEMORY_FULL__

/// @brief namespace related to memory functions
namespace kernel::memory {

typedef struct __KM_HEAP_BLOCK {
    size_t size;
    bool free;
    __KM_HEAP_BLOCK* next;
    __KM_HEAP_BLOCK* prev;
} heap_block_t;

typedef struct __KM_HEAP {
    void* start;
    uint64_t size;
    heap_block_t* first_block;
} heap_t;

/// @brief          checks the memory alignment of a given address
/// @param addr     address to check
/// @param align    target alignment
/// @return         result of the alignment check
bool
check_alignment(
    uint64_t addr,
    uint64_t align);

/// @brief          aligns a address to the given alignment (always rounds upwards)
/// @param addr     address to align
/// @param align    target alignment
/// @return         aligned address
uint64_t
align_up_to(
    uint64_t addr,
    uint64_t align);

/// @brief namespace for paging related functions
namespace paging {

/// @brief       get a new physical page
/// @throws      kernel panic when no more pages are available
/// @returns     pointer to physical memory in "user" space
void*
get_page();

/// @brief       get a new physical page
/// @throws      kernel panic when no more pages are available
/// @returns     pointer to physical memory in "kernel" space
void*
get_page_kernel();

/// @brief              reserves a page at a certain memory address
///                     autmatically dertemines kernel | user-kernel maps
/// @param address      memory address to map
void
page_reserve_at_index(
    uint64_t address);

/// @brief              reserves memory pages so get_page cannot get them
/// @throws             kernel panic when no more pages are available
/// @param[in] count    amount of pages to reserve
void
page_reserve(
    size_t count);

/// @brief              reserves memory pages so get_page cannot get them
/// @throws             kernel panic when no more pages are available
/// @param[in] count    amount of pages to reserve
void
page_reserve_kernel(
    size_t count);

/// @brief                      maps a 1K memory page
/// @param[in] pml4t            pointer to pml4 struct
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param[in] physical_addr    pointer to physical address to map to
void
map_small_page(
    uint64_t* pml4t,
    void* virtual_addr,
    void* physical_addr);

/// @brief                      maps a 2M memory page
/// @param[in] pml4t            pointer to pml4 struct
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param[in] physical_addr    pointer to physical address to map to
/// @details                    the remainder of the pages have to be manually reserved
void
map_large_page(
    uint64_t* pml4t,
    void* virtual_addr,
    void* physical_addr);

/// @brief                      returns the physical address from the virtual address
/// @param[in] virtual_addr     virtual address that needs to be converted
/// @return                     0 if invalid virtual address else physical address
uint64_t
virtual_to_physical(
    uint64_t* pml4t,
    void* virtual_addr);

} // namespace paging

/// @brief namespace for virtual memory related functions
namespace vmem {

/// @brief                          initializes memory
/// @param[inout] heap              pointer to a heap struct
/// @param[inout] pml4t             pointer to a valid PML4 table
/// @param[in]    multiboot_info    pointer to multiboot structure
/// @details                        this function reassigns the pml4 that the cpu wants
///                                 it also sets the global heap & pml4 pointer for kalloc & kfree
void
init(
    heap_t* heap,
    uint64_t* pml4t,
    void* multiboot_info);

/// @brief                      initializes the kernel heap to a specific size
/// @param[inout] heap          pointer to a heap struct
/// @param[inout] pml4t         pointer to a PML4 table
/// @param[in] start_adress     pointer to the start of virtual memory for the heap
/// @param size                 starting size in B
/// @details                    will allocate more pages if needed
void
init_kernel_heap(
    heap_t* heap,
    uint64_t* pml4t,
    void* start_adress,
    size_t size);

/// @brief                  expands the heap by X size
/// @param[inout] heap      pointer to a heap struct
/// @param[inout] pml4t     pointer to a PML4 table
/// @param size             size to add to the heap
/// @details                will allocate more pages if needed
void
kernel_heap_expand(
    heap_t* heap,
    uint64_t* pml4t,
    uint64_t size);

/// @brief          allocates a new block of memory
/// @param size     size of new block
/// @return         pointer to new block or nullptr if no more memory is available
void*
kalloc(
    size_t size);

/// @brief              free's a block of memory & cleans the heap
/// @param[in] ptr      pointer to memory block
void
kfree(
    void* ptr);

} // namespace vmem
} // namespace kernel::memory

void* operator new(size_t size);
// void* operator new[](size_t size);

void* operator new(size_t size, void* ptr);
// void* operator new[](size_t size, void* ptr);

void operator delete(void* ptr);
// void operator delete[](void* ptr);

void operator delete(void* ptr, size_t size);
// void operator delete[](void* ptr, size_t size);

template <typename _class>
class kernel_unique_ptr {
public:
    kernel_unique_ptr(size_t count = 1) : m_count(count) {
        m_ptr = (_class*)kernel::memory::vmem::kalloc(sizeof(_class) * m_count);
        for (size_t i = 0; i < m_count; ++i)
            new (m_ptr + i) _class();
    };

    kernel_unique_ptr(const kernel_unique_ptr& other) = delete;
    kernel_unique_ptr& operator=(const kernel_unique_ptr& other) = delete;
    kernel_unique_ptr(kernel_unique_ptr&& other) = delete;
    kernel_unique_ptr& operator=(kernel_unique_ptr&& other) = delete;

    _class* get() const {
        return m_ptr;
    }

    void free() const {
        ~kernel_unique_ptr();
    }
    
    ~kernel_unique_ptr() {
        if (m_ptr) {
            for (size_t i = 0; i < m_count; ++i)
                (m_ptr + i)->~_class();

            kernel::memory::vmem::kfree(m_ptr);
        }
    };

private:
    _class* m_ptr = nullptr;
    size_t m_count = 0;
};

template <typename _type>
struct remove_extent {
    using type = _type;
};

template <typename _type, size_t _size>
struct remove_extent<_type[_size]> {
    using type = _type;
};

template <typename _type>
struct remove_extent<_type[]> {
    using type = _type;
};

template <typename _type>
using remove_extent_t = typename remove_extent<_type>::type;

template <typename _class>
auto kernel_make_unique_ptr(const size_t size) {
    return kernel_unique_ptr<remove_extent_t<_class>>(size);
}

template <typename _class>
auto kernel_make_unique_ptr() {
    return kernel_unique_ptr<_class>();
}

#endif // __KERNEL_MEMORY_FULL__

#endif // __MEMORY_HPP__
