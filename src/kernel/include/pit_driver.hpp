//==========================================
/// @file       pit_driver.hpp
/// @brief      pit related functions
//==========================================

#pragma once

#ifndef __PIT_DRIVER_HPP__
#define __PIT_DRIVER_HPP__

#define CLOCK_1MS 1000

#include "common.hpp"
#include "interrupt.hpp"
#include "vector.hpp"

struct pit_timer_t {
    uint64_t id;
    uint64_t tick;
};

cpu_state_t* pit_handle_interrupt(uint64_t code, cpu_state_t* rsp);

void pit_sleep(uint32_t ms);
void pit_sleep(uint64_t id, uint32_t ms);
void pit_init(vector<pit_timer_t>* timers);
void pit_add_clock(uint64_t id);

#endif // __PIT_DRIVER_HPP__