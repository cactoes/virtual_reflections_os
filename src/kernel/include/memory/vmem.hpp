//==========================================
/// @file       vmem.hpp
/// @brief      virtual memory
//==========================================

#pragma once

#ifndef __MEMORY_VMEM_HPP__
#define __MEMORY_VMEM_HPP__

// addresses are based on kernel location, if the kernel is bigger than 256mb it will go wrong
#define VMEM_DMA_ALLOCATOR_START    0xFFFFF80100000000  // 256mb
#define VMEM_MAPPED_MMIO_REGION     0xFFFFF80200000000  // 256mb
#define VMEM_KERNEL_HEAP_START      0xFFFFF80300000000  // remainder

#define VMEM_BIT_USER_KERNEL        0
#define VMEM_BIT_READWRITE          1
#define VMEM_BIT_EXECUTE            2

#define VMEM_USER       (0 << VMEM_BIT_USER_KERNEL)
#define VMEM_KERNEL     (1 << VMEM_BIT_USER_KERNEL)
#define VMEM_READWRITE  (1 << VMEM_BIT_READWRITE)
#define VMEM_EXECUTE    (1 << VMEM_BIT_EXECUTE)

#include "common.hpp"

/// @brief                  maps a 2mb page to a virtual address from a phisical address
/// @param[in] vaddr        pointer to virtual address to map to
/// @param[in] paddr        pointer to physical address to map to
/// @return                 success status
bool vmem_map_2mb(const void* vaddr, const void* paddr, u64 flags);

/// @brief                      allocates memory pages per 2mb if size != large page
///                             it will round up too the nearest 2mb
/// @param[in] vaddr            pointer to virtual address to map to
/// @param size                 memory size to allocate
/// @return                     allocated amount of memory
size_t vmem_smart_alloc_pages(const void* vaddr, size_t size, u64 flags);

/// @brief                      initiates virtual memory
/// @param[inout] pml4          page table to use
/// @param[in] multiboot_struct pointer to the custom mb struct for memory regions
/// @return                     success status
bool vmem_init(const void* mbstruct);

/// @brief                      memory maps an region to virtual memory
/// @param physical_address     the address to map
/// @return                     the virtual address its mapped to if success else nullptr
void* vmem_map_mmio_region(void* paddr);

void* vmem_virtual_to_physical(void* vaddr);

/// @brief                      unmaps a 2mb page from a virtual address
/// @param vaddr                virtual address to unmap
/// @return                     success status
bool vmem_unmap_2mb(void* vaddr);

#endif // __MEMORY_VMEM_HPP__