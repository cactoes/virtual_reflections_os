//==========================================
/// @file       clock.hpp
/// @brief      system clocks etc
//==========================================

#pragma once

#ifndef __TIME_CLOCK_HPP__
#define __TIME_CLOCK_HPP__

#include "common.hpp"

u64 clock_get_time_since_boot();
u64 clock_get_current_time();

#endif // __TIME_CLOCK_HPP__