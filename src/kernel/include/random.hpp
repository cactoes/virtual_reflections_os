//==========================================
/// @file       random.hpp
/// @brief      random engine of the kernel
//==========================================

#pragma once

#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__

#include "common.hpp"

void seed_random(uint64_t seed);
uint64_t random_number();

#endif // __RANDOM_HPP__