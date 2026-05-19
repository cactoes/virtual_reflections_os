#include "arch/amd64/pit.hpp"
#include "arch/amd64/port.hpp"

void amd64_pit_init(u16 times_per_second) {
    amd64_out_port8(0x43, 0x36);
    u16 count = 1193182 / times_per_second;

    amd64_out_port8(0x40, (u8)(count & 0xFF));
    amd64_out_port8(0x40, (u8)((count >> 8) & 0xFF));
}