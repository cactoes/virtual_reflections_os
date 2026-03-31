//==========================================
/// @file       gdt.hpp
/// @brief      x86_64 gdt impl
//==========================================

#pragma once

#ifndef __X86_64_GDT_HPP__
#define __X86_64_GDT_HPP__

#define X86_64_GDT_ACCESS_PRESENT      (1 << 7)
#define X86_64_GDT_ACCESS_RING0        (0 << 0)
// #define X86_64_GDT_ACCESS_RING1        0b00100000
// #define X86_64_GDT_ACCESS_RING2        0b01000000
#define X86_64_GDT_ACCESS_RING3        ((1 << 5) | (1 << 6))
#define X86_64_GDT_ACCESS_SEGMENT      (1 << 4)
#define X86_64_GDT_ACCESS_EXECUTABLE   (1 << 3)
#define X86_64_GDT_ACCESS_DIRECTION    (1 << 2)
#define X86_64_GDT_ACCESS_READWRITE    (1 << 1)
#define X86_64_GDT_ACCESS_ACCESSED     (1 << 0)

#define X86_64_GDT_GRANULARITY_4K      (1 << 7)
#define X86_64_GDT_GRANULARITY_1B      (0 << 0)
#define X86_64_GDT_OPERAND_SIZE_32     (1 << 6)
#define X86_64_GDT_LONG_MODE           (1 << 5)
#define X86_64_GDT_AVAILABLE           (1 << 4)

#define X86_64_GDT_INDEX_TSS(c)                 sizeof(x86_64_gdt_t<c>::entries) / sizeof(x86_64_gdt_entry_t)
#define X86_64_GDT_INDEX_TO_ENTRY(index)       (index) * sizeof(x86_64_gdt_entry_t)

#include "common.hpp"

struct x86_64_gdt_entry_t {
    uint16_t limit;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high8;
};

struct x86_64_tss_entry_t {
    uint16_t length;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base_high8;
    uint32_t base_upper32;
    uint32_t reserved;
};

template <size_t entry_count>
struct x86_64_gdt_t {
    x86_64_gdt_entry_t entries[entry_count];
    x86_64_tss_entry_t tss_entry;
};

struct x86_64_gdtr_t {
    uint16_t limit;
    uint64_t address;
} PACKED;

struct x86_64_tss_t {
    uint32_t resereved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t resereved1;
    uint64_t resereved2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t resereved3;
    uint16_t resereved4;
    uint16_t iomap_offset;
} PACKED;

static x86_64_gdt_entry_t g_x86_64_zero_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = 0,
    .granularity = 0,
    .base_high8 = 0,
};

static x86_64_gdt_entry_t g_x86_64_kernel_64_code_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = X86_64_GDT_ACCESS_PRESENT | X86_64_GDT_ACCESS_RING0 |
              X86_64_GDT_ACCESS_SEGMENT | X86_64_GDT_ACCESS_EXECUTABLE |
              X86_64_GDT_ACCESS_READWRITE,
    .granularity = X86_64_GDT_LONG_MODE,
    .base_high8 = 0,
};

static x86_64_gdt_entry_t g_x86_64_kernel_64_data_entry {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = X86_64_GDT_ACCESS_PRESENT | X86_64_GDT_ACCESS_RING0 | 
              X86_64_GDT_ACCESS_SEGMENT | X86_64_GDT_ACCESS_READWRITE,
    .granularity = 0, 
    .base_high8 = 0,
};

static x86_64_gdt_entry_t g_x86_64_user_64_code_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = X86_64_GDT_ACCESS_PRESENT | X86_64_GDT_ACCESS_RING3 |
              X86_64_GDT_ACCESS_SEGMENT | X86_64_GDT_ACCESS_EXECUTABLE |
              X86_64_GDT_ACCESS_READWRITE,
    .granularity = X86_64_GDT_LONG_MODE,
    .base_high8 = 0,
};

static x86_64_gdt_entry_t g_x86_64_user_64_data_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = X86_64_GDT_ACCESS_PRESENT | X86_64_GDT_ACCESS_RING3 |
              X86_64_GDT_ACCESS_SEGMENT | X86_64_GDT_ACCESS_READWRITE,
    .granularity = 0,
    .base_high8 = 0,
};

template <size_t entry_count>
void x86_64_gdt_init(x86_64_gdt_t<entry_count>* p_gdt) {
    memzero(p_gdt, sizeof(x86_64_gdt_t<entry_count>));
}

template <size_t entry_count>
void x86_64_gdt_set_entry(x86_64_gdt_t<entry_count>* p_gdt, x86_64_gdt_entry_t* p_entry, size_t index) {
    if (index >= entry_count)
        return;

    memcpy(&p_gdt->entries[index], p_entry, sizeof(x86_64_gdt_entry_t));
}

template <size_t entry_count>
void x86_64_gdt_set_tss(x86_64_gdt_t<entry_count>* p_gdt, x86_64_tss_t* p_tss) {
    p_gdt->tss_entry.length = sizeof(x86_64_tss_t) - 1;
    p_gdt->tss_entry.base_low16 = (uint16_t)(((uint64_t)p_tss) & MAX_UINT16);
    p_gdt->tss_entry.base_mid8 = (uint8_t)(((uint64_t)p_tss >> 16) & MAX_UINT8);
    p_gdt->tss_entry.flags1 = 0b10001001;
    p_gdt->tss_entry.flags2 = 0;
    p_gdt->tss_entry.base_high8 = (uint8_t)(((uint64_t)p_tss >> 24) & MAX_UINT8);
    p_gdt->tss_entry.base_upper32 = (uint32_t) (((uint64_t)p_tss >> 32) & MAX_UINT32);
    p_gdt->tss_entry.reserved = 0;
}

void x86_64_tss_set_stack_pointer0(x86_64_tss_t* p_tss, void* p_stack_pointer);

template <size_t entry_count>
void x86_64_gdtr_update(x86_64_gdtr_t* p_gdtr, x86_64_gdt_t<entry_count>* p_gdt) {
    p_gdtr->limit = sizeof(x86_64_gdt_t<entry_count>) - 1;
    p_gdtr->address = (uint64_t)p_gdt;
}

#endif // __X86_64_GDT_HPP__