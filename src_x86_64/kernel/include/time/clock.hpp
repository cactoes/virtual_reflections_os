//==========================================
/// @file       clock.hpp
/// @brief      system clocks etc
//==========================================

#pragma once

#ifndef __TIME_CLOCK_HPP__
#define __TIME_CLOCK_HPP__

#include "common.hpp"

uint64_t clock_get_time_since_boot();
uint64_t clock_get_current_time();

#endif // __TIME_CLOCK_HPP__