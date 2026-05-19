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

cpu_t* get_current_cpu();
bool cpu_set_kernel_stack(cpu_t* cpu, void* stack);

#endif // __CPU_HPP__