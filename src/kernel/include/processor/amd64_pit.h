//==========================================
/// @file       amd64_pit.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_PIT_H
#define AMD64_PIT_H

#define AMD64_PIT_HZ             1193182ull
#define AMD64_PIT_POLL_LIMIT     1000000u

#include "libtype.h"

bool pit_delay_us(u32 us);

#endif // AMD64_PIT_H