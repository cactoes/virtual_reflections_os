//==========================================
/// @file       vmem.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AMD64_VMEM_HPP__
#define __AMD64_VMEM_HPP__

#include "common.hpp"

bool amd64_vmem_map_2mb(const void* vaddr, const void* paddr, bool is_kernel, bool is_readwrite, bool is_executeable);
bool amd64_vmem_unmap_2mb(void* vaddr);
void* amd64_vmem_virtual_to_physical(void* vaddr);

static inline
void amd64_flush_tlb(void* vaddr) {
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

#endif // __AMD64_VMEM_HPP__