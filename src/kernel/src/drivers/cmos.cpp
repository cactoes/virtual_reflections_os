#include "drivers/cmos.hpp"

// TODO @since 21/05/2026 -- 22:29
// you know the drill
#include "arch/amd64/port.hpp"

static int days_in_month(int year, int month) {
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2)
        return days[1] + (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

    return days[month - 1];
}

u8 cmos_read(u8 reg) {
    amd64_out_port8(CMOS_ADDR, reg);
    return amd64_in_port8(CMOS_DATA);
}

u64 cmos_read_time() {
    while (cmos_read(CMOS_REG_STATUS_A) & CMOS_UIP);

    u8 second = cmos_read(CMOS_REG_SECONDS);
    u8 minute = cmos_read(CMOS_REG_MINUTES);
    u8 hour = cmos_read(CMOS_REG_HOURS);
    u8 day = cmos_read(CMOS_REG_DAY_OF_MONTH);
    u8 month = cmos_read(CMOS_REG_MONTH);
    u8 year = cmos_read(CMOS_REG_YEAR);
    u8 regB = cmos_read(CMOS_REG_STATUS_B);

    if (!(regB & 0x04)) {
        second = BCD_TO_BIN(second);
        minute = BCD_TO_BIN(minute);
        hour = BCD_TO_BIN(hour & 0x7F);
        day = BCD_TO_BIN(day);
        month = BCD_TO_BIN(month);
        year = BCD_TO_BIN(year);
    }

    int full_year = 2000 + year;
    if (full_year < 1970) full_year += 100;

    u64 days = 0;
    for (int y = 1970; y < full_year; y++)
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;

    for (int m = 1; m < month; m++)
        days += days_in_month(full_year, m);

    days += (day - 1);

    u64 seconds = days * 86400ULL + (u64)hour * 3600ULL + (u64)minute * 60ULL + second;
    return seconds * 1000ULL;
}