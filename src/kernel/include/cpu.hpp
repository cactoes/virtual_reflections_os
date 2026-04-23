//==========================================
/// @file       cpu.hpp
/// @brief      cpu basic logic
//==========================================

#pragma once

#ifndef __CPU_HPP__
#define __CPU_HPP__

#include "common.hpp"

struct interrupt_regs_t {
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;

    uint64_t error_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} ALIGNED(16) PACKED;

struct syscall_regs_t {
    uint64_t r15, r14, r13, r12, rbp, rbx, r10, r9, r8, rdx, rsi, rdi;
    uint64_t r11, rcx;
} ALIGNED(16) PACKED;

struct cpu_t {
    uint64_t kernel_rsp;
    uint64_t user_rsp;
} PACKED;

bool initialize_cpus();
cpu_t* get_current_cpu();
bool set_kernel_stack(cpu_t* cpu, void* stack);

#endif // __CPU_HPP__