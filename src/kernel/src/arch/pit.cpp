#include "arch/pit.hpp"
#include "arch/generic.hpp"

void pit_init(u64 times_per_s) {
    out_port<u8>(0x43, 0x36);
    u64 count = 1193182 / times_per_s;

    out_port<u8>(0x40, (u8)(count & 0xFF));
    out_port<u8>(0x40, (u8)((count >> 8) & 0xFF));
}