//==========================================
/// @file       pit.hpp
/// @brief      programmable interupt timer
//==========================================

#pragma once

#ifndef __PIT_HPP__
#define __PIT_HPP__

#define ARCH_X86_64
#ifdef ARCH_X86_64

#include "common.hpp"

void pit_init(u64 times_per_s);

#endif // ARCH_X86_64

#endif // __PIT_HPP__