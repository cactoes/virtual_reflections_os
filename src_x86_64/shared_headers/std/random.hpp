//==========================================
/// @file       random.hpp
/// @brief      random engine of the kernel
//==========================================

#pragma once

#ifndef __RANDOM_HPP__
#define __RANDOM_HPP__

#include "common.hpp"

static uint64_t global_random_seed = 0;

/// @brief          seeds the random number generator with the specified seed
/// @param seed     the seed value to initialize the random number generator
static void seed_random(uint64_t seed) {
    global_random_seed = seed;
}

/// @brief          returns a random 64-bit number
/// @return         random uint64_t value
static uint64_t random_number() {
    global_random_seed = global_random_seed * 6364136223846793005 + 1;
    return global_random_seed;
}

/// @brief          generates a random number in the given range
/// @param min      minimum value (inclusive)
/// @param max      maximum value (exclusive)
/// @return         random number in [min, max)
static uint64_t random_number(uint64_t min, uint64_t max) {
    return (random_number() % (max - min + 1)) + min;
}

#endif // __RANDOM_HPP__