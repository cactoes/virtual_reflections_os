//==========================================
/// @file       random.hpp
/// @brief      random engine of the kernel
//==========================================

#pragma once

#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__

#include "common.hpp"

/// @brief          seeds the random number generator with the specified seed
/// @param seed     the seed value to initialize the random number generator
void seed_random(uint64_t seed);

/// @brief          returns a random 64-bit number
/// @return         random uint64_t value
uint64_t random_number();

/// @brief          generates a random number in the given range
/// @param min      minimum value (inclusive)
/// @param max      maximum value (exclusive)
/// @return         random number in [min, max)
uint64_t random_number(uint64_t min, uint64_t max);

#endif // __RANDOM_HPP__