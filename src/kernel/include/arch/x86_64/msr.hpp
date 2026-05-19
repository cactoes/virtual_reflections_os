// //==========================================
// /// @file       msr.hpp
// /// @brief      x86_64 msr impl
// //==========================================

// #pragma once

// #ifndef __X86_64_MSR_HPP__
// #define __X86_64_MSR_HPP__

// #define X86_64_MSR_EFER            0xC0000080
// #define X86_64_MSR_STAR            0xC0000081
// #define X86_64_MSR_LSTAR           0xC0000082
// #define X86_64_MSR_CSTAR           0xC0000083
// #define X86_64_MSR_SFMASK          0xC0000084
// #define X86_64_MSR_FS_BASE         0xC0000100
// #define X86_64_MSR_GS_BASE         0xC0000101
// #define X86_64_MSR_KERNEL_GS_BASE  0xC0000102

// #define X86_64_EFER_SCE            (1 << 0)
// #define X86_64_EFER_LME            (1 << 8)
// #define X86_64_EFER_LMA            (1 << 10)
// #define X86_64_EFER_NXE            (1 << 11)

// #include "common.hpp"
// #include "arch/x86_64/generic.hpp"

// static inline void x86_64_msr_set_kernel_gs_base(void* addr) {
//     x86_64_wrmsr(X86_64_MSR_KERNEL_GS_BASE, (u64)addr);
// }

// static inline void x86_64_msr_set_gs_base(void* addr) {
//     x86_64_wrmsr(X86_64_MSR_GS_BASE, (u64)addr);
// }

// static inline void x86_64_msr_enable_sce() {
//     u64 efer = x86_64_rdmsr(X86_64_MSR_EFER);
//     x86_64_wrmsr(X86_64_MSR_EFER, efer | X86_64_EFER_SCE);
// }

// static inline void x86_64_msr_set_star(u64 star) {
//     x86_64_wrmsr(X86_64_MSR_STAR, star);
// }

// static inline void x86_64_msr_set_lstar(void* lstar) {
//     x86_64_wrmsr(X86_64_MSR_LSTAR, (u64)lstar);
// }

// static inline void x86_64_msr_set_sf_mask(u64 mask) {
//     x86_64_wrmsr(X86_64_MSR_SFMASK, mask);
// }

// #endif // __X86_64_MSR_HPP__