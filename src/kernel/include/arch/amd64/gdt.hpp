//==========================================
/// @file       gdt.hpp
/// @brief      gdt
//==========================================

#pragma once

#ifndef __AMD64_GDT_HPP__
#define __AMD64_GDT_HPP__

#define AMD64_GDT_ACCESS_PRESENT            (1 << 7)
#define AMD64_GDT_ACCESS_RING0              (0 << 0)
#define AMD64_GDT_ACCESS_RING1              0b00100000
#define AMD64_GDT_ACCESS_RING2              0b01000000
#define AMD64_GDT_ACCESS_RING3              ((1 << 5) | (1 << 6))
#define AMD64_GDT_ACCESS_SEGMENT            (1 << 4)
#define AMD64_GDT_ACCESS_EXECUTABLE         (1 << 3)
#define AMD64_GDT_ACCESS_DIRECTION          (1 << 2)
#define AMD64_GDT_ACCESS_READWRITE          (1 << 1)
#define AMD64_GDT_ACCESS_ACCESSED           (1 << 0)

#define AMD64_GDT_GRANULARITY_4K            (1 << 7)
#define AMD64_GDT_GRANULARITY_1B            (0 << 0)
#define AMD64_GDT_OPERAND_SIZE_32           (1 << 6)
#define AMD64_GDT_LONG_MODE                 (1 << 5)
#define AMD64_GDT_AVAILABLE                 (1 << 4)

#define AMD64_GDT_INDEX_TSS(c)              sizeof(amd64_gdt_t::entries) / sizeof(amd64_gdt_entry_t)
#define AMD64_GDT_INDEX_TO_ENTRY(index)     (index) * sizeof(amd64_gdt_entry_t)

#include "common.hpp"

enum {
    GDT_ZERO_ENTRY = 0,
    KERNEL_CODE_SELECTOR_INDEX,
    KERNEL_DATA_SELECTOR_INDEX,
    USER_CODE_32_SELECTOR_INDEX,
    USER_DATA_SELECTOR_INDEX,
    USER_CODE_SELECTOR_INDEX,

    GDT_ENTRY_COUNT
};

struct amd64_gdt_entry_t {
    u16 limit;
    u16 base_low16;
    u8 base_mid8;
    u8 access;
    u8 granularity;
    u8 base_high8;
};

struct amd64_tss_entry_t {
    u16 length;
    u16 base_low16;
    u8 base_mid8;
    u8 flags1;
    u8 flags2;
    u8 base_high8;
    u32 base_upper32;
    u32 reserved;
};

struct amd64_gdt_t {
    amd64_gdt_entry_t entries[GDT_ENTRY_COUNT];
    amd64_tss_entry_t tss_entry;
};

struct amd64_gdtr_t {
    u16 limit;
    u64 address;
} PACKED;

struct amd64_tss_t {
    u32 resereved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 resereved1;
    u64 resereved2;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 resereved3;
    u16 resereved4;
    u16 iomap_offset;
} PACKED;

static amd64_gdt_entry_t g_amd64_zero_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = 0,
    .granularity = 0,
    .base_high8 = 0,
};

static amd64_gdt_entry_t g_amd64_kernel_64_code_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING0 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_EXECUTABLE |
              AMD64_GDT_ACCESS_READWRITE,
    .granularity = AMD64_GDT_LONG_MODE,
    .base_high8 = 0,
};

static amd64_gdt_entry_t g_amd64_kernel_64_data_entry {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING0 | 
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_READWRITE,
    .granularity = 0, 
    .base_high8 = 0,
};

static amd64_gdt_entry_t g_amd64_user_32_code_entry = {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING3 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_EXECUTABLE |
              AMD64_GDT_ACCESS_READWRITE,
    .granularity = 0,
    .base_high8 = 0,
};

static amd64_gdt_entry_t g_amd64_user_64_code_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING3 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_EXECUTABLE |
              AMD64_GDT_ACCESS_READWRITE,
    .granularity = AMD64_GDT_LONG_MODE,
    .base_high8 = 0,
};

static amd64_gdt_entry_t g_amd64_user_64_data_entry  {
    .limit = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING3 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_READWRITE,
    .granularity = 0,
    .base_high8 = 0,
};

void amd64_gdt_init(amd64_gdt_t* gdt);
void amd64_gdt_set_entry(amd64_gdt_t* gdt, amd64_gdt_entry_t* entry, u64 index);
void amd64_gdt_set_tss(amd64_gdt_t* gdt, amd64_tss_t* tss);
void amd64_tss_set_stack_pointer0(amd64_tss_t* tss, void* stack_pointer);
void amd64_gdtr_update(amd64_gdtr_t* gdtr, amd64_gdt_t* gdt);

u16 amd64_get_selector_for(u16 index);

static inline void amd64_set_gdt(void* gdtr) {
    asm volatile("lgdt (%0)" : : "r"(gdtr) : "memory");
}

static inline void amd64_load_tss(u16 entry) {
    asm volatile(
        "mov %0, %%ax\n"
        "ltr %%ax\n"
        :
        : "r"(entry)
        : "rax", "memory"
    );
}

static inline void amd64_reload_segments(u16 code_selector, u16 data_selector) {
    asm volatile (
        "mov %w0, %%ds\n"
        "mov %w0, %%es\n"
        "mov %w0, %%ss\n"
        "mov %w0, %%fs\n"
        "mov %w0, %%gs\n"
        
        "pushq %q1\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        : 
        : "r" (data_selector), "r" ((u64)code_selector)
        : "rax", "memory"
    );
}

#endif // __AMD64_GDT_HPP__