//==========================================
/// @file       cmos.hpp
/// @brief      cmos
//==========================================

#pragma once

#ifndef __DRIVERS_CMOS_HPP__
#define __DRIVERS_CMOS_HPP__

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define CMOS_REG_SECONDS        0x00
#define CMOS_REG_ALARM_SECONDS  0x01
#define CMOS_REG_MINUTES        0x02
#define CMOS_REG_ALARM_MINUTES  0x03
#define CMOS_REG_HOURS          0x04
#define CMOS_REG_ALARM_HOURS    0x05
#define CMOS_REG_WEEKDAY        0x06
#define CMOS_REG_DAY_OF_MONTH   0x07
#define CMOS_REG_MONTH          0x08
#define CMOS_REG_YEAR           0x09
#define CMOS_REG_STATUS_A       0x0A
#define CMOS_REG_STATUS_B       0x0B
#define CMOS_REG_STATUS_C       0x0C
#define CMOS_REG_STATUS_D       0x0D
#define CMOS_REG_CENTURY        0x32

#define CMOS_UIP                0x80

#include "common.hpp"

u8 cmos_read(u8 reg);
u64 cmos_read_time();

#endif // __DRIVERS_CMOS_HPP__