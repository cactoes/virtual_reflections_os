//==========================================
/// @file       vmem.hpp
/// @brief      virtual memory
//==========================================

#pragma once

#ifndef __MEMORY_VMEM_HPP__
#define __MEMORY_VMEM_HPP__

#define VMEM_HEAP_START_ADDR      0x40000000
#define VMEM_E1000_DMA            (VMEM_HEAP_START_ADDR - PAGE_SIZE_LARGE)
#define VMEM_E1000_MMIO           (VMEM_E1000_DMA - PAGE_SIZE_LARGE)
#define VMEM_AHCI_DMA             (VMEM_E1000_MMIO - PAGE_SIZE_LARGE)
#define VMEM_AHCI_MMIO            (VMEM_AHCI_DMA - PAGE_SIZE_LARGE)

#include "common.hpp"

/// @brief              maps the firtst ~1gb one-one in the page table
///                     so that those values are always valid
/// @param[inout] pml4  pointer to the page table struct
void vmem_identity_map(void* p_pml4);

/// @brief                      maps a 4kb page to a virtual address from a phisical address
/// @param[inout] pml4          page table to use
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param[in] physical_addr    pointer to physical address to map to
/// @return                     success status
NODISCARD bool vmem_map_2kb_page(void* p_pml4, void* p_virtual_addr, void* p_physical_addr);

/// @brief                      maps a 2mb page to a virtual address from a phisical address
/// @param[inout] pml4          page table to use
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param[in] physical_addr    pointer to physical address to map to
/// @return                     success status
NODISCARD bool vmem_map_2mb_page(void* p_pml4, void* p_virtual_addr, void* p_physical_addr);

/// @brief                      allocates memory pages per 2mb if size != large page
///                             it will round up too the nearest 2mb
/// @param[inout] pml4          page table to use
/// @param[in] virtual_addr     pointer to virtual address to map to
/// @param size                 memory size to allocate
/// @return                     allocated amount of memory
NODISCARD size_t vmem_smart_alloc_pages(void* pml4, void* virtual_addr, size_t size);

NODISCARD void* vmem_virtual_to_physical(void* p_pml4, void* p_virtual_addr);

/// @brief                      initiates virtual memory
/// @param[in] multiboot_struct pointer to the custom mb struct for memory regions
/// @param[inout] pml4          page table to use
/// @return                     success status
NODISCARD bool vmem_init(void* p_multiboot_struct, void* p_pml4);

#endif // __MEMORY_VMEM_HPP__