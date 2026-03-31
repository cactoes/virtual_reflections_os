#include "arch/pit.hpp"
#include "arch/generic.hpp"

void pit_init(uint64_t times_per_s) {
    out_port<uint8_t>(0x43, 0x36);
    uint64_t count = 1193182 / times_per_s;

    out_port<uint8_t>(0x40, (uint8_t)(count & 0xFF));
    out_port<uint8_t>(0x40, (uint8_t)((count >> 8) & 0xFF));
}