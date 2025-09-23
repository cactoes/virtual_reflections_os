#include "random.hpp"
#include "time/clock.hpp"

static uint64_t g_random_seed = 0;

void seed_random(uint64_t seed) {
    g_random_seed = seed;
}

uint64_t random_number() {
    g_random_seed = g_random_seed * 6364136223846793005 + 1;
    return g_random_seed;
}

uint64_t random_number(uint64_t min, uint64_t max) {
    return (random_number() % (max - min + 1)) + min;
}