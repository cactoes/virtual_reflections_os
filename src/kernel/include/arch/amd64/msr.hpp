//==========================================
/// @file       msr.hpp
/// @brief      msr
//==========================================
#pragma once

#ifndef __AMD64_MSR_HPP__
#define __AMD64_MSR_HPP__

#define AMD64_MSR_EFER            0xC0000080
#define AMD64_MSR_STAR            0xC0000081
#define AMD64_MSR_LSTAR           0xC0000082
#define AMD64_MSR_CSTAR           0xC0000083
#define AMD64_MSR_SFMASK          0xC0000084
#define AMD64_MSR_FS_BASE         0xC0000100
#define AMD64_MSR_GS_BASE         0xC0000101
#define AMD64_MSR_KERNEL_GS_BASE  0xC0000102

#define AMD64_EFER_SCE            (1 << 0)
#define AMD64_EFER_LME            (1 << 8)
#define AMD64_EFER_LMA            (1 << 10)
#define AMD64_EFER_NXE            (1 << 11)

#include "common2.hpp"

static inline
void amd64_wrmsr(u32 addr, u64 value) {
    u32 low  = (u32)(value & 0xFFFFFFFF);
    u32 high = (u32)(value >> 32);

    asm volatile ("wrmsr" :: "c"(addr), "a"(low), "d"(high));
}

static inline
u64 amd64_rdmsr(u32 addr) {
    u32 low, high;

    asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(addr));

    return ((u64)high << 32) | low;
}

static inline
void amd64_msr_set_kernel_gs_base(void* address) {
    amd64_wrmsr(AMD64_MSR_KERNEL_GS_BASE, (u64)address);
}

static inline
void amd64_msr_set_gs_base(void* addr) {
    amd64_wrmsr(AMD64_MSR_GS_BASE, (u64)addr);
}

static inline
void amd64_msr_enable_sce() {
    u64 efer = amd64_rdmsr(AMD64_MSR_EFER);
    amd64_wrmsr(AMD64_MSR_EFER, efer | AMD64_EFER_SCE);
}

static inline
void amd64_msr_set_star(u64 star) {
    amd64_wrmsr(AMD64_MSR_STAR, star);
}

static inline
void amd64_msr_set_lstar(void* lstar) {
    amd64_wrmsr(AMD64_MSR_LSTAR, (u64)lstar);
}

static inline
void amd64_msr_set_sf_mask(u64 mask) {
    amd64_wrmsr(AMD64_MSR_SFMASK, mask);
}

#endif // __AMD64_MSR_HPP__