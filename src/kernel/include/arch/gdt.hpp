//==========================================
/// @file       gdt.hpp
/// @brief      gdt implementation
//==========================================

#pragma once

#ifndef __GDT_HPP__
#define __GDT_HPP__

#include "common.hpp"

#define ARCH_X86_64
#ifdef ARCH_X86_64

// enum {
//     GDT_ZERO_ENTRY = 0,
//     KERNEL_CODE_SELECTOR_INDEX,
//     KERNEL_DATA_SELECTOR_INDEX,
//     USER_CODE_32_SELECTOR_INDEX,
//     USER_DATA_SELECTOR_INDEX,
//     USER_CODE_SELECTOR_INDEX,

//     GDT_ENTRY_COUNT
// };

// #include "arch/x86_64/gdt.hpp"

// void gdt_init();
// void gdt_set_stack_pointer0(void* p_stack_pointer);
// uint64_t gdt_get_kernel_code_selector();
// uint64_t gdt_get_kernel_data_selector();

#endif // ARCH_X86_64

#endif // __GDT_HPP__