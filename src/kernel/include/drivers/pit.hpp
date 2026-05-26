//==========================================
/// @file       pit.hpp
/// @brief      programmable interrupt timer impl
//==========================================

#pragma once

#ifndef __DRIVERS_PIT_HPP__
#define __DRIVERS_PIT_HPP__

#include "common.hpp"

typedef void*(*pit_interrupt_function_t)(void*, void*);

void* pit_handle_interrupt(void* p_cpu_state, void*);
u64 pit_get_global_tick_count();
void pit_init(u16 times_per_second);

#endif // __DRIVERS_PIT_HPP__