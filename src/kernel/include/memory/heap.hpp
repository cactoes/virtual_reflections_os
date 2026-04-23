//==========================================
/// @file       heap.hpp
/// @brief      heap impl
//==========================================

#pragma once

#ifndef __MEMORY_HEAP_HPP__
#define __MEMORY_HEAP_HPP__

// heap block filter helpers
#define HEAP_MAKE_FILTER_PARAM(param) ((void*)&param)
#define HEAP_FILTERS_SIZE(array) ARRAY_LENGTH(array)

// heap helper functions / macros
// #define GALLOC(size) malloc(size)
// #define GFREE(ptr) free(ptr)
// #define GLOBAL_HEAP get_global_heap()

#include "common.hpp"
#include "std/array.hpp"

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
    void* pml4;
};

struct dma_heap_manager_t {
    void* start_virtual_addr;
    size_t size;
    void* pml4;
    std::dynamic_array<heap_t> heaps;
};

typedef bool(*heap_block_filter_callback_t)(const heap_block_t*, const void*);

/// @brief      filters for block finding / filtering (undocumented filters)
namespace heap_block_filters {

bool donor_block_filter(const heap_block_t* p_block, const void* p_param);
bool unused_block_filter(const heap_block_t* p_block, const void* p_param);
bool last_block_filter(const heap_block_t* p_block, const void* p_param);

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
int heap_filter_blocks(heap_t* p_heap, void* p_param, heap_block_filter_callback_t p_hbfc_array[], size_t bfc_array_size, heap_block_t* p_block_array[], size_t block_array_size);

/// @brief                      initiates a heap
/// @param[inout] heap          heap to initiate
/// @param[inout] pml4          page table to use
/// @param[in] vaddr            virtual address to start the heap from
/// @param size                 starting size of the heap
/// @return                     success status
bool heap_init(heap_t* heap, void* pml4, void* vaddr, size_t size, bool is_user = false);

/// @brief              expands the heap ontop of current heap
///                     make sure the virtual address above is not yet reserved
/// @param[inout] heap  heap to expand
/// @param[inout] pml4  page table to use
/// @param size         size to expand the heap by
/// @return             success status
NODISCARD bool heap_expand(heap_t* p_heap, size_t size);

/// @brief              allocates memory from a heap
/// @param[in] heap     heap to allocate from
/// @param size         memory block size
/// @return             ptr to memory or nullptr if no more memory is available
void* heap_alloc(heap_t* p_heap, size_t size);

/// @brief              deallocates memory from a heap
/// @param[in] heap     heap to free from
/// @param[in] ptr      pointer to memory
void heap_free(heap_t* p_heap, void* p_ptr);

/// @brief                      allocates a aligned mem block
/// @param[inout] dma_heap      pointer to the dma_heap_t struct
/// @return                     ptr to the block
void* dma_heap_alloc(heap_t* p_dma_heap, size_t size, uint64_t align);

/// @brief                      frees the given block
/// @param[inout] dma_heap      ptr to the dma_heap_t struct where the block is located
/// @param[in] block            ptr to the block to free
void dma_heap_free(heap_t* p_dma_heap, void* p_block);

/// @brief                  converts a virtual addr in the dma heap to a physical addr
/// @param[inout] dma_heap  heap that contains the block
/// @param[in] block        ptr to the block / the virtual addr
/// @return                 physical addr
uint64_t dma_get_physical(heap_t* p_dma_heap, void* p_block);

/// @brief                  converts a virtual addr in the dma heap to a physical addr
///                         lower 32 bits
/// @param[inout] dma_heap  heap that contains the block
/// @param[in] block        ptr to the block / the virtual addr
/// @return                 physical addr lower 32 bits
uint32_t dma_get_physical_lower(heap_t* p_dma_heap, void* p_block);

/// @brief                  converts a virtual addr in the dma heap to a physical addr
///                         upper 32 bits
/// @param[inout] dma_heap  heap that contains the block
/// @param[in] block        ptr to the block / the virtual addr
/// @return                 physical addr upper 32 bits
uint32_t dma_get_physical_upper(heap_t* p_dma_heap, void* p_block);

/// @brief                          initializes a dma heap
/// @param[inout] pml4              ptr to the page table to use
/// @param[inout] dma_heap          ptr to the dma_heap_t
/// @param[in] virtual_address      
/// @param size                     
/// @return                         0 success, 1 incorrect alignment, 2 heap size failed, 3 heap init failed
/// @remarks                        technically memory leaks since it never frees
///                                 the memory used for the entire dma heap
NODISCARD int dma_heap_init(void* p_pml4, heap_t* p_dma_heap, void* p_virtual_address, size_t size);

/// @brief                      initialzes a dma heap manager
/// @param[inout] manager       pointer to the dma heap manager struct
/// @param[inout] pml4          corresponding page table
/// @param[in] virtual_address  virtual address to assign dma heaps to
/// @param size                 total size for dma heaps
/// @return                     success status
bool dma_heap_manager_init(dma_heap_manager_t* manager, void* pml4, void* virtual_address, size_t size);

/// @brief              sets the global dma heap manager to the given one
/// @param[in] manager  pointer to the dma heap manager struct that should be global
void set_global_dma_heap_manager(dma_heap_manager_t* manager);

/// @brief              gets the global dma heap manager if set
/// @return             pointer to the global dma heap manager
dma_heap_manager_t* get_global_dma_heap_manager();

/// @brief                  creates a dma heap
/// @param[inout] manager   pointer to the dma heap manager
/// @param size             size of the dma heap
/// @return                 pointer to heap or nullptr if failed
heap_t* dma_heap_manager_create_heap(dma_heap_manager_t* manager, size_t size);

/// @brief              sets the global heap to the given one
/// @param[in] heap     pointer to heap that should be global
void set_global_heap(heap_t* p_heap);

/// @brief      gets the global heap if set
/// @return     pointer to global heap
heap_t* get_global_heap();

void* malloc(size_t size) noexcept;
void free(void* ptr) noexcept;

void* malloc_aligned(size_t size, size_t align) noexcept;
void free_aligned(void* ptr) noexcept;

void* operator new(__SIZE_TYPE__ size) noexcept;
void* operator new(__SIZE_TYPE__ size, void* p_ptr) noexcept;
void* operator new[](__SIZE_TYPE__ size) noexcept;
void* operator new[](__SIZE_TYPE__ size, void*) noexcept;

void operator delete(void* p_ptr) noexcept;
void operator delete(void* p_ptr, __SIZE_TYPE__) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete[](void* ptr, __SIZE_TYPE__) noexcept;

#endif // __MEMORY_HEAP_HPP__