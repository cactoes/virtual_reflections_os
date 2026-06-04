//==========================================
/// @file       cpu.hpp
/// @brief      cpu basic logic
//==========================================

#pragma once

#ifndef __CPU_HPP__
#define __CPU_HPP__

#include "common.hpp"

struct cpu_t {
    u64 kernel_rsp;
    u64 user_rsp;
} __attribute__((packed));

cpu_t* get_current_cpu();
bool cpu_set_kernel_stack(cpu_t* cpu, void* stack);
bool cpu_get_name(char* buffer, u64 size);

#endif // __CPU_HPP__