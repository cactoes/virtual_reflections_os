//==========================================
/// @file       pit.hpp
/// @brief      programmable interrupt timer impl
//==========================================

#pragma once

#ifndef __DRIVERS_PIT_HPP__
#define __DRIVERS_PIT_HPP__

#include "arch/pit.hpp"
#include "common.hpp"
#include "cpu.hpp"

typedef interrupt_regs_t*(*pit_interrupt_function_t)(interrupt_regs_t*, void*);

interrupt_regs_t* pit_handle_interrupt(interrupt_regs_t* p_cpu_state, void*);
uint64_t pit_get_global_tick_count();
void pit_add_interrupt_function(pit_interrupt_function_t p_function);

#endif // __DRIVERS_PIT_HPP__