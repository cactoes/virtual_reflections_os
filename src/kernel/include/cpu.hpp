//==========================================
/// @file       cpu.hpp
/// @brief      cpu basic logic
//==========================================

#pragma once

#ifndef __CPU_HPP__
#define __CPU_HPP__

#include "common.hpp"

// TODO @since 19/05/2026 -- 13:11
// IFDEF FOR ARCHITECTURE

#include "arch/amd64/cpu.hpp"

struct syscall_regs_t {
    u64 r15, r14, r13, r12, rbp, rbx, r10, r9, r8, rdx, rsi, rdi;
    u64 r11, rcx;
} ALIGNED(16) PACKED;

struct cpu_t {
    u64 kernel_rsp;
    u64 user_rsp;
} PACKED;

bool initialize_cpus();
cpu_t* get_current_cpu();
bool set_kernel_stack(cpu_t* cpu, void* stack);

#endif // __CPU_HPP__