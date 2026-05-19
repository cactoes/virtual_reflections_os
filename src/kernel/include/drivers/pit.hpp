//==========================================
/// @file       pit.hpp
/// @brief      programmable interrupt timer impl
//==========================================

#pragma once

#ifndef __DRIVERS_PIT_HPP__
#define __DRIVERS_PIT_HPP__

#include "common.hpp"
#include "cpu.hpp"

typedef interrupt_regs_t*(*pit_interrupt_function_t)(interrupt_regs_t*, void*);

interrupt_regs_t* pit_handle_interrupt(interrupt_regs_t* p_cpu_state, void*);
u64 pit_get_global_tick_count();
void pit_add_interrupt_function(pit_interrupt_function_t p_function);
void pit_init(u16 times_per_second);

#endif // __DRIVERS_PIT_HPP__