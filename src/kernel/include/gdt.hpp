//==========================================
/// @file       gdt.hpp
/// @brief      global descriptor table stuff
//==========================================

#pragma once

#ifndef __GDT_HPP__
#define __GDT_HPP__

#define GDT_ACCESS_PRESENT      0b10000000
#define GDT_ACCESS_RING0        0b00000000
// #define GDT_ACCESS_RING1        0b00100000
// #define GDT_ACCESS_RING2        0b01000000
#define GDT_ACCESS_RING3        0b01100000
#define GDT_ACCESS_SEGMENT      0b00010000
#define GDT_ACCESS_EXECUTABLE   0b00001000
#define GDT_ACCESS_DIRECTION    0b00000100
#define GDT_ACCESS_READWRITE    0b00000010
#define GDT_ACCESS_ACCESSED     0b00000001

#define GDT_GRANULARITY_4K      0b10000000
#define GDT_GRANULARITY_1B      0b00000000
#define GDT_OPERAND_SIZE_32     0b01000000
#define GDT_LONG_MODE           0b00100000
#define GDT_AVAILABLE           0b00010000

#define GDT_INDEX_NULL                  0
#define GDT_INDEX_KERNEL_64_CODE        1
#define GDT_INDEX_TSS                   sizeof(gdt::entries) / sizeof(gdt_entry_t)

#define GDT_INDEX_TO_ENTRY(index)       (index) * sizeof(gdt_entry_t)

#include "common.hpp"

struct gdt_entry_t {
    uint16_t limit;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high8;
};

struct tss_entry_t {
    uint16_t length;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base_high8;
    uint32_t base_upper32;
    uint32_t reserved;
};

struct gdt {
    gdt_entry_t entries[2];
    tss_entry_t tss_entry;
};

struct gdtr {
    uint16_t limit;
    uint64_t address;
} PACKED;

struct tss_t {
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

void gdt_init();
void tss_init();
void tss_set_rsp0(void* rsp);

#endif // __GDT_HPP__