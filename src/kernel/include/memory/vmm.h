//==========================================
/// @file       vmm.h
/// @brief      
//==========================================

#pragma once

#ifndef VMM_H
#define VMM_H

#define VMEM_PAGE_SIZE_SMALL          0x1000
#define VMEM_PAGE_SIZE_LARGE    0x200000
#define VMEM_PAGE_SIZE_HUGE     0x40000000

#define VMEM_WRITE          (1ull << 0)
#define VMEM_EXEC           (1ull << 1)
#define VMEM_USER           (1ull << 2)
#define VMEM_GLOBAL         (1ull << 3)

#define VMEM_CACHE_SHIFT    4
#define VMEM_CACHE_MASK     (3ull << VMEM_CACHE_SHIFT)
#define VMEM_CACHE_WB       (0ull << VMEM_CACHE_SHIFT)
#define VMEM_CACHE_WT       (1ull << VMEM_CACHE_SHIFT)
#define VMEM_CACHE_WC       (2ull << VMEM_CACHE_SHIFT)
#define VMEM_CACHE_UC       (3ull << VMEM_CACHE_SHIFT)

#define VMEM_KERNEL_DATA    (VMEM_WRITE | VMEM_GLOBAL)
#define VMEM_KERNEL_CODE    (VMEM_EXEC | VMEM_GLOBAL)
#define VMEM_KERNEL_RO      (VMEM_GLOBAL)
#define VMEM_MMIO           (VMEM_WRITE | VMEM_CACHE_UC)
#define VMEM_FRAMEBUFFER    (VMEM_WRITE | VMEM_CACHE_WC)
#define VMEM_USER_DATA      (VMEM_USER | VMEM_WRITE)
#define VMEM_USER_CODE      (VMEM_USER | VMEM_EXEC)

#include "libtype.h"

bool vmm_map_1gb(const void* vaddr, const void* paddr, u64 flags);
bool vmm_map_2mb(const void* vaddr, const void* paddr, u64 flags);
bool vmm_map_4kb(const void* vaddr, const void* paddr, u64 flags);

bool vmm_unmap_1gb(const void* vaddr, void* old_paddr);
bool vmm_unmap_2mb(const void* vaddr, void* old_paddr);
bool vmm_unmap_4kb(const void* vaddr, void* old_paddr);

#endif // VMM_H