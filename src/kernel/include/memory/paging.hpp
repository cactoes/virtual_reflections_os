//==========================================
/// @file       paging.hpp
/// @brief      memory paging impl
//==========================================

#pragma once

#ifndef __MEMORY_PAGING_HPP__
#define __MEMORY_PAGING_HPP__

// MAKE SURE TO KEEP IN LINE WITH SYSTEM RAM
// OR REPLACE THIS WITH ACTUAL SYSTEM RAM CHECKING
#define KERNEL_PAGE_RESERVED_BITMAP_SIZE    0x2000 // 1024mb    0x2000 * 64 = page count
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

#include "common.hpp"

/// @brief      gets a physical address in the reserved section
/// @return     physical memroy address
void* pmem_get_page_reserved();

/// @brief      gets a physical address after the reserved section
/// @return     physical memroy address
void* pmem_get_page();

/// @brief              sets bitmap values so that those addresses
///                     can no longer be used for getting physical pages.
///                     automatically determines the bitmap
/// @param address      address to start reserving from (4k aligned)
/// @param count        amount of pages to reserve at that address
/// @return             success status
NODISCARD bool pmem_reserve_at_adress(uint64_t address, size_t count = 1);

bool pmem_is_in_memory_range(void* p_addr);

#endif // __MEMORY_PAGING_HPP__