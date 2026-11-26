#include "processor/amd64_pit.h"
#include "processor/amd64_port.h"
#include "processor/amd64_cpu.h"

bool pit_delay_us(u32 us) {
    while (us != 0) {
        const u32 chunk = us > 50000u ? 50000u : us;
        const u32 pit_ticks = (u32)(((u64)chunk * AMD64_PIT_HZ + 999999ull) / 1000000ull);
        const u8 saved_port = amd64_in_port8(0x61);

        amd64_out_port8(0x61, (u8)(saved_port & ~3u));
        amd64_out_port8(0x43, 0xb0);
        amd64_out_port8(0x42, (u8)pit_ticks);
        amd64_out_port8(0x42, (u8)(pit_ticks >> 8));
        amd64_out_port8(0x61, (u8)((saved_port & ~2u) | 1u));

        bool elapsed = false;
        for (u32 poll = 0; poll < AMD64_PIT_POLL_LIMIT; poll++) {
            if (amd64_in_port8(0x61) & (1u << 5)) {
                elapsed = true;
                break;
            }

            amd64_pause();
        }

        amd64_out_port8(0x61, saved_port);
        if (!elapsed)
            return false;

        us -= chunk;
    }

    return true;
}