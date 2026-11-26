//==========================================
/// @file       amd64_ap.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_AP_H
#define AMD64_AP_H

#define AP_TRAMPOLINE_PHYS 0x8000ull
#define AP_DATA_OFFSET     0x300ull
#define AP_PAGE_SIZE       0x1000ull

#define SMP_AP_TIMEOUT_MS      1000u
#define SMP_IPI_INIT_ASSERT    ((5u << 8) | (1u << 14) | (1u << 15))
#define SMP_IPI_INIT_DEASSERT  ((5u << 8) | (1u << 15))
#define SMP_IPI_STARTUP        (6u << 8)

#include "libtype.h"
#include "kbootinfo.h"

typedef struct {
    u64 boot_cr3;
    u64 kernel_cr3;
    u64 stack_top;
    u64 cpu;
    u64 entry;
    u64 extensions;
    u64 pat;
} ap_boot_data_t;

struct __cpu;

void amd64_ap_entry(struct __cpu* cpu, u64 extensions);
bool amd64_ap_prepare(kboot_info_t* kboot_info);
bool amd64_ap_start_and_ping(u64 cpu_index);
void amd64_ap_startup(kboot_info_t* kboot_info, u64 cpu_count);
bool amd64_ap_send_ipi(u64 cpu_index, u32 vector);

#endif // AMD64_AP_H