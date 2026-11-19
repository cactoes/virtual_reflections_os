//==========================================
/// @file       amd64_gdt.h
/// @brief      this currently only works for the boot cpu
///             not for extra cores
//==========================================

#pragma once

#ifndef AMD64_GDT_H
#define AMD64_GDT_H
#define AMD64_GDT_ACCESS_PRESENT            (1u << 7)
#define AMD64_GDT_ACCESS_RING0              (0u << 0)
#define AMD64_GDT_ACCESS_RING1              (1u << 5)
#define AMD64_GDT_ACCESS_RING2              (1u << 6)
#define AMD64_GDT_ACCESS_RING3              ((1u << 5) | (1u << 6))
#define AMD64_GDT_ACCESS_SEGMENT            (1u << 4)
#define AMD64_GDT_ACCESS_EXECUTABLE         (1u << 3)
#define AMD64_GDT_ACCESS_DIRECTION          (1u << 2)
#define AMD64_GDT_ACCESS_READWRITE          (1u << 1)
#define AMD64_GDT_ACCESS_ACCESSED           (1u << 0)

#define AMD64_GDT_GRANULARITY_4K            (1u << 7)
#define AMD64_GDT_GRANULARITY_1B            (0u << 0)
#define AMD64_GDT_OPERAND_SIZE_32           (1u << 6)
#define AMD64_GDT_LONG_MODE                 (1u << 5)
#define AMD64_GDT_AVAILABLE                 (1u << 4)

#define AMD64_GDT_TYPE_TSS64_AVAILABLE      0x9u

#define AMD64_GDT_INDEX_TO_SELECTOR(index, rpl)  ((u16)(((index) << 3) | (rpl)))

#include "libdef.h"
#include "libtype.h"

typedef enum __amd64_gdt_indexes {
    GDT_INDEX_ZERO_ENTRY = 0,
    GDT_INDEX_KERNEL_CODE,
    GDT_INDEX_KERNEL_DATA,
    GDT_INDEX_USER_CODE32,
    GDT_INDEX_USER_DATA,
    GDT_INDEX_USER_CODE,
    GDT_SEGMENT_COUNT,
    GDT_INDEX_TSS = GDT_SEGMENT_COUNT
} amd64_gdt_index_t;

typedef struct __amd64_gdt_entry {
    u16 limit_low16;
    u16 base_low16;
    u8 base_mid8;
    u8 access;
    u8 limit_high4_flags;
    u8 base_high8;
} __packed amd64_gdt_entry_t;

typedef struct __amd64_tss_descriptor {
    u16 limit_low16;
    u16 base_low16;
    u8 base_mid8;
    u8 access;
    u8 limit_high4_flags;
    u8 base_high8;
    u32 base_upper32;
    u32 reserved;
} __packed amd64_tss_descriptor_t;

typedef struct __amd64_gdt {
    amd64_gdt_entry_t entries[GDT_SEGMENT_COUNT];
    amd64_tss_descriptor_t tss;
} __align_packed(16) amd64_gdt_t;

typedef struct __amd64_gdtr {
    u16 limit;
    u64 address;
} __packed amd64_gdtr_t;

typedef struct __amd64_tss {
    u32 reserved0;
    u64 rsp[3];
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iopb_offset;
} __packed amd64_tss_t;

void amd64_cpu_gdt_init();

#endif // AMD64_GDT_H