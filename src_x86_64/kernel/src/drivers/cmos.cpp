#include "drivers/cmos.hpp"

#include "arch/generic.hpp"

static int days_in_month(int year, int month) {
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2)
        return days[1] + (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

    return days[month - 1];
}

uint8_t cmos_read(uint8_t reg) {
    out_port<uint8_t>(CMOS_ADDR, reg);
    return in_port<uint8_t>(CMOS_DATA);
}

uint64_t cmos_read_time() {
    while (cmos_read(CMOS_REG_STATUS_A) & CMOS_UIP);

    uint8_t second = cmos_read(CMOS_REG_SECONDS);
    uint8_t minute = cmos_read(CMOS_REG_MINUTES);
    uint8_t hour = cmos_read(CMOS_REG_HOURS);
    uint8_t day = cmos_read(CMOS_REG_DAY_OF_MONTH);
    uint8_t month = cmos_read(CMOS_REG_MONTH);
    uint8_t year = cmos_read(CMOS_REG_YEAR);
    uint8_t regB = cmos_read(CMOS_REG_STATUS_B);

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

    uint64_t days = 0;
    for (int y = 1970; y < full_year; y++)
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;

    for (int m = 1; m < month; m++)
        days += days_in_month(full_year, m);

    days += (day - 1);

    uint64_t seconds = days * 86400ULL + (uint64_t)hour * 3600ULL + (uint64_t)minute * 60ULL + second;
    return seconds * 1000ULL;
}