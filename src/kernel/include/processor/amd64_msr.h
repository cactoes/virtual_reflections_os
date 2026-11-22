//==========================================
/// @file       amd64_msr.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_MSR_H
#define AMD64_MSR_H

#define AMD64_EFER_SCE      (1u << 0)
#define AMD64_EFER_LME      (1u << 8)
#define AMD64_EFER_LMA      (1u << 10)
#define AMD64_EFER_NXE      (1u << 11)

#define AMD64_MSR_EFER              0xc0000080u
#define AMD64_MSR_STAR              0xc0000081u
#define AMD64_MSR_LSTAR             0xc0000082u
#define AMD64_MSR_CSTAR             0xc0000083u
#define AMD64_MSR_SFMASK            0xc0000084u
#define AMD64_MSR_FS_BASE           0xc0000100u
#define AMD64_MSR_GS_BASE           0xc0000101u
#define AMD64_MSR_KERNEL_GS_BASE    0xc0000102u

#define AMD64_MSR_PAT           0x277u
#define AMD64_MSR_LAPIC_BASE    0x1B

#define AMD64_MSR_LAPIC_ENABLE    (1 << 11)

#include "libtype.h"

static inline
void amd64_wrmsr(u32 addr, u64 value) {
    u32 low  = (u32)(value & 0xffffffff);
    u32 high = (u32)(value >> 32);

    __asm__ volatile ("wrmsr" :: "c"(addr), "a"(low), "d"(high) : "memory");
}

static inline
u64 amd64_rdmsr(u32 addr) {
    u32 low, high;

    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(addr) : "memory");

    return ((u64)high << 32) | low;
}

#endif // AMD64_MSR_H