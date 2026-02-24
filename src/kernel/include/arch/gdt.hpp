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

#define GDT_ENTRY_COUNT 5
#define KERNEL_CODE_SELECTOR_INDEX 1
#define KERNEL_DATA_SELECTOR_INDEX 2
#define USER_CODE_SELECTOR_INDEX 4
#define USER_DATA_SELECTOR_INDEX 3

#include "arch/x86_64/gdt.hpp"

void gdt_init();
void gdt_set_stack_pointer0(void* p_stack_pointer);
uint64_t gdt_get_kernel_code_selector();

#endif // ARCH_X86_64

#endif // __GDT_HPP__