//==========================================
/// @file       cpu.hpp
/// @brief      amd64 implementation of the cpu registers etc
//==========================================

#pragma once

#ifndef __AMD64_CPU_HPP__
#define __AMD64_CPU_HPP__

#include "common.hpp"
#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

struct interrupt_regs_t {
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;

    u64 error_code;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __attribute__((aligned(16), packed));

struct syscall_regs_t {
    u64 r15, r14, r13, r12, rbp, rbx, r10, r9, r8, rdx, rsi, rdi;
    u64 r11, rcx;
} __attribute__((aligned(16), packed));

static inline
void amd64_cpuid(u32 eax_in, u32 ecx_in, u32* eax_out, u32* ebx_out, u32* ecx_out, u32* edx_out) {
    asm volatile ("cpuid" : "=a"(*eax_out), "=b"(*ebx_out), "=c"(*ecx_out), "=d"(*edx_out) : "a"(eax_in), "c"(ecx_in));
}

static inline
void amd64_halt() {
    asm volatile ("hlt");
}

static inline
void amd64_pause() {
    asm volatile ("pause");
}

static inline
void amd64_mem_barier() {
    asm volatile ("" ::: "memory");
}

static inline
u64 amd64_read_cr2() {
    u64 res;
    asm volatile("mov %%cr2, %0" : "=r"(res));
    return res;
}

static inline
u64 amd64_save_flags_and_cli() {
    u64 flags;
    asm volatile(
        "pushfq\n"
        "pop %0\n"
        "cli\n"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

static inline
void amd64_restore_flags(u64 flags) {
    asm volatile(
        "push %0\n"
        "popfq"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

#endif

#endif // __AMD64_CPU_HPP__