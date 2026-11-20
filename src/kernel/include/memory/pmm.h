//==========================================
/// @file       pmm.h
/// @brief      
//==========================================

#pragma once

#ifndef PMM_H
#define PMM_H

#define PMM_PAGE_SIZE       4096ull
#define PMM_MAX_MEMORY      (4ull * 1024 * 1024 * 1024)
#define PMM_MAX_PAGES       (PMM_MAX_MEMORY / PMM_PAGE_SIZE)

#include "libtype.h"

bool pmm_reserve_range(void* addr, u64 count);
void* pmm_alloc_pages(u64 count);
void pmm_free_pages(void* addr, u64 count);
void pmm_init(u64 max_physical_pages);

#endif // PMM_H