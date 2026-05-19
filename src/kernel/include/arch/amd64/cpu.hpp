//==========================================
/// @file       cpu.hpp
/// @brief      amd64 implementation of the cpu registers etc
//==========================================

#pragma once

#ifndef __AMD64_CPU_HPP__
#define __AMD64_CPU_HPP__

#include "common.hpp"

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

struct cpu_t {
    u64 kernel_rsp;
    u64 user_rsp;
} __attribute__((packed));

#endif // __AMD64_CPU_HPP__