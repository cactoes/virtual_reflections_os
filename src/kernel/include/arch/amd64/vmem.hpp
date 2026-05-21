//==========================================
/// @file       vmem.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AMD64_VMEM_HPP__
#define __AMD64_VMEM_HPP__

#include "common.hpp"
#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

bool amd64_vmem_map_2mb(const void* vaddr, const void* paddr, bool is_kernel, bool is_readwrite, bool is_executeable);
bool amd64_vmem_unmap_2mb(void* vaddr);
void* amd64_vmem_virtual_to_physical(void* vaddr);

static inline
void amd64_flush_tlb(void* vaddr) {
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static inline
void amd64_set_page_table(void* page_table) {
    asm volatile("mov %0, %%cr3" : : "r"(page_table) : "memory");
}

static inline
void* amd64_get_page_table() {
    void* page_table;
    asm volatile("mov %%cr3, %0" : "=r"(page_table) : : "memory");
    return page_table;
}

static inline
void amd64_fpu_store(void* fpu_region) {
    asm volatile("fxsave (%0)" :: "r"(fpu_region));
}

static inline
void amd64_fpu_load(void* fpu_region) {
    asm volatile("fxrstor (%0)" :: "r"(fpu_region));
}

#endif

#endif // __AMD64_VMEM_HPP__