//==========================================
/// @file       pit.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AMD64_PIT_HPP__
#define __AMD64_PIT_HPP__

#include "common.hpp"
#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

void amd64_pit_init(u16 times_per_second);

#endif

#endif // __AMD64_PIT_HPP__