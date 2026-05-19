// //==========================================
// /// @file       msr.hpp
// /// @brief      msr functions
// //==========================================

// #pragma once

// #ifndef __MSR_HPP__
// #define __MSR_HPP__

// #define ARCH_X86_64
// #ifdef ARCH_X86_64

// #include "arch/x86_64/msr.hpp"

// static inline void msr_set_kernel_gs_base(void* addr) {
//     x86_64_msr_set_kernel_gs_base(addr);
// }

// static inline void msr_set_gs_base(void* addr) {
//     x86_64_msr_set_gs_base(addr);
// }

// static inline void msr_enable_sce() {
//     x86_64_msr_enable_sce();
// }

// static inline void msr_set_star(uint64_t star) {
//     x86_64_msr_set_star(star);
// }

// static inline void msr_set_lstar(void* lstar) {
//     x86_64_msr_set_lstar(lstar);
// }

// static inline void msr_set_sf_mask(uint64_t mask) {
//     x86_64_msr_set_sf_mask(mask);
// }

// #endif

// #endif // __MSR_HPP__