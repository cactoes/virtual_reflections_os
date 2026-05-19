//==========================================
/// @file       paging.hpp
/// @brief      memory paging impl
//==========================================

#pragma once

#ifndef __MEMORY_PAGING_HPP__
#define __MEMORY_PAGING_HPP__

// chosen randomly, if system ram is above 4gb it wont support it
// TODO @since 16/01/2026 -- 20:12
// dynamic page tracker
#define PAGING_BITMAP_SIZE                  0x4000 // 4096mb    0x4000 * 64 = page count

// helper functions for page tables
#define KPAGING_GET_PE(virtual_addr, offset)    ((((u64)(virtual_addr)) >> (offset)) & 0x1FF)
#define KPAGING_GET_ENTRY(table, entry)         ((u64*)(((u64*)(table))[((u64)(entry))] & ~0xFFF))
#define KPAGING_CHECK_ENTRY(table, entry)       ((((u64*)(table))[((u64)(entry))]) & 1)

#define RECURSIVE_SLOT 511ULL
#define SIGN_EXT 0xFFFF000000000000ULL
#define GET_PML4_VIRT()                                     ((u64*)(SIGN_EXT | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30)         | (RECURSIVE_SLOT << 21)            | (RECURSIVE_SLOT << 12)))
#define GET_PDPT_VIRT(pml4_index)                           ((u64*)(SIGN_EXT | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30)         | (RECURSIVE_SLOT << 21)            | ((u64)(pml4_index) << 12)))
#define GET_PDT_VIRT(pml4_index, pdpt_index)                ((u64*)(SIGN_EXT | (RECURSIVE_SLOT << 39) | (RECURSIVE_SLOT << 30)         | ((u64)(pml4_index) << 21)    | ((u64)(pdpt_index) << 12)))
#define GET_PT_VIRT(pml4_index, pdpt_index, pdt_index)      ((u64*)(SIGN_EXT | (RECURSIVE_SLOT << 39) | ((u64)(pml4_index) << 30) | ((u64)(pdpt_index) << 21)    | ((u64)(pdt_index) << 12)))

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

/// @brief      gets a physical address
/// @return     physical memroy address
void* pmem_get_page();

/// @brief              sets bitmap values so that those addresses
///                     can no longer be used for getting physical pages
/// @param[in] paddr    address to start reserving from (4k aligned)
/// @param count        amount of pages to reserve at that address
/// @return             success status
bool pmem_try_reserve_address(const void* paddr, size_t count = 1);

bool pmem_is_in_memory_range(const void* address);

#endif // __MEMORY_PAGING_HPP__