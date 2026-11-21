//==========================================
/// @file       amd64_vmm.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_VMM_H
#define AMD64_VMM_H

#define AMD64_PTE_PRESENT    (1ull << 0)
#define AMD64_PTE_WRITE      (1ull << 1)
#define AMD64_PTE_USER       (1ull << 2)
#define AMD64_PTE_PWT        (1ull << 3)
#define AMD64_PTE_PCD        (1ull << 4)
#define AMD64_PTE_HUGE       (1ull << 7)
#define AMD64_PTE_PAT_4K     (1ull << 7)
#define AMD64_PTE_GLOBAL     (1ull << 8)
#define AMD64_PTE_PAT_LARGE  (1ull << 12)
#define AMD64_PTE_NX         (1ull << 63)
 
#define AMD64_PTE_ADDR_MASK_4K  0x000ffffffffff000ull
#define AMD64_PTE_ADDR_MASK_2M  0x000fffffffe00000ull
#define AMD64_PTE_ADDR_MASK_1G  0x000fffffc0000000ull

#define AMD64_KPAGING_GET_PE(virtual_addr, offset)    ((((u64)(virtual_addr)) >> (offset)) & 0x1ff)
#define AMD64_KPAGING_GET_ENTRY(table, entry)         ((u64*)(((u64*)(table))[((u64)(entry))] & ~0xfff))
#define AMD64_KPAGING_CHECK_ENTRY(table, entry)       ((((u64*)(table))[((u64)(entry))]) & 1)

#define AMD64_RECURSIVE_SLOT 511ull
#define AMD64_SIGN_EXT 0xffff000000000000ull
#define AMD64_GET_PML4_VIRT()                                     ((u64*)(AMD64_SIGN_EXT | (AMD64_RECURSIVE_SLOT << 39) | (AMD64_RECURSIVE_SLOT << 30) | (AMD64_RECURSIVE_SLOT << 21) | (AMD64_RECURSIVE_SLOT << 12)))
#define AMD64_GET_PDPT_VIRT(pml4_index)                           ((u64*)(AMD64_SIGN_EXT | (AMD64_RECURSIVE_SLOT << 39) | (AMD64_RECURSIVE_SLOT << 30) | (AMD64_RECURSIVE_SLOT << 21) | ((u64)(pml4_index) << 12)))
#define AMD64_GET_PDT_VIRT(pml4_index, pdpt_index)                ((u64*)(AMD64_SIGN_EXT | (AMD64_RECURSIVE_SLOT << 39) | (AMD64_RECURSIVE_SLOT << 30) | ((u64)(pml4_index) << 21) | ((u64)(pdpt_index) << 12)))
#define AMD64_GET_PT_VIRT(pml4_index, pdpt_index, pdt_index)      ((u64*)(AMD64_SIGN_EXT | (AMD64_RECURSIVE_SLOT << 39) | ((u64)(pml4_index) << 30) | ((u64)(pdpt_index) << 21) | ((u64)(pdt_index) << 12)))

#define AMD64_PF_PRESENT              (1 << 0)
#define AMD64_PF_READ_WRITE           (1 << 1)
#define AMD64_PF_USER_SUPERVISOR      (1 << 2)
#define AMD64_PF_WRITE_THROUGH        (1 << 3)
#define AMD64_PF_CACHE_DISABLE        (1 << 4)
#define AMD64_PF_ACCESSED             (1 << 5)
#define AMD64_PF_DIRTY                (1 << 6)
#define AMD64_PF_PAGE_SIZE            (1 << 7)
#define AMD64_PF_GLOBAL               (1 << 8)

#include "libtype.h"

u64 amd64_vmm_flags_to_pte(u64 flags, u64 page_size, bool nx_enabled);
bool amd64_vmm_page_already_mapped(const void* vaddr, u64 page_size);
bool amd64_vmm_install_virtual_address(const void* vaddr, const void* paddr, u64 page_size, u64 flags);
bool amd64_vmm_remove_mapping(const void* vaddr, u64 page_size, u64* old_paddr);
void amd64_vmm_set_use_nx(bool state);
bool amd64_vmm_get_use_nx();

#endif // AMD64_VMM_H