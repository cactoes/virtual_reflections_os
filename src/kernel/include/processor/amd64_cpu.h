//==========================================
/// @file       amd64_cpu.h
/// @brief      am64 (x86-64) related cpu functions
//==========================================

#pragma once

#ifndef AMD64_CPU_H
#define AMD64_CPU_H

#define AMD64_CR0_MP              (1u << 1)
#define AMD64_CR0_EM              (1u << 2)
 
#define AMD64_CR4_OSFXSR          (1u << 9)
#define AMD64_CR4_OSXMMEXCPT      (1u << 10)

#define AMD64_CPUID1_EDX_SIMD       ((1u << 24) | (1u << 25) | (1u << 26)) // FXSR, SSE, SSE2
#define AMD64_CPUID1_ECX_SIMD       ((1u << 0) | (1u << 9) | (1u << 19) | (1u << 20) | (1u << 28)) // SSE3, SSSE3, SSE4.1, SSE4.2, AVX

#define AMD64_CPUID_LEAF_BASIC_MAX          0x00000000
#define AMD64_CPUID_LEAF_FEATURES           0x00000001
#define AMD64_CPUID_LEAF_EXTENDED_FEAT      0x00000007
#define AMD64_CPUID_LEAF_EXTENDED_MAX       0x80000000
#define AMD64_CPUID_LEAF_EXTENDED_INFO      0x80000001

#define AMD64_CPU_EXTENSION_SIMD_BIT    0

void amd64_enable_processor_extensions(u64 extensions);
u64 amd64_get_processor_extensions();

static inline
void amd64_disable_interrupts() {
    __asm__ volatile(
        "cli"
        :
        :
        : "memory"
    );
}

static inline
void amd64_enable_interrupts() {
    __asm__ volatile(
        "sti"
        :
        :
        : "memory"
    );
}

static inline
u64 amd64_save_interrupts() {
    u64 flags;
    __asm__ volatile(
        "pushfq\n"
        "popq %0\n"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

static inline
void amd64_restore_interrupts(u64 flags) {
    if (flags & (1u << 9)) // IF
        amd64_enable_interrupts();
}

#endif // AMD64_CPU_H