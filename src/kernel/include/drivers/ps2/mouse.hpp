//==========================================
/// @file       mouse.hpp
/// @brief      ps2 mouse driver
//==========================================

#pragma once

#ifndef __DRIVERS_PS2_MOUSE_HPP__
#define __DRIVERS_PS2_MOUSE_HPP__

#define PS2_MOUSE_CMD_PORT                0x64
#define PS2_MOUSE_DATA_PORT               0x60

#define PS2_MOUSE_ENABLE_AUX_DEVICE       0xA8
#define PS2_MOUSE_READ_CMD_BYTE           0x20
#define PS2_MOUSE_WRITE_CMD_BYTE          0x60
#define PS2_MOUSE_WRITE_TO_MOUSE          0xD4

#define PS2_MOUSE_MOUSE_SET_DEFAULTS      0xF6
#define PS2_MOUSE_MOUSE_ENABLE_REPORTING  0xF4
#define PS2_MOUSE_CMD_SET_SAMPLE_RATE     0xF3

#define PS2_MOUSE_CMD_ENABLE_MOUSE_IRQ    0x02

#define PS2_MOUSE_SAMPLE_RATE_1    200
#define PS2_MOUSE_SAMPLE_RATE_2    100
#define PS2_MOUSE_SAMPLE_RATE_3     80

#include "common.hpp"

struct ps2_mouse_state_t {
    int dx;
    int dy;
    int ds;

    struct {
        bool left : 1;
        bool middle : 1;
        bool right : 1;
    } buttons;
} PACKED;

void ps2_mouse_init();
void ps2_mouse_handle_interrupt();

#endif // __DRIVERS_PS2_MOUSE_HPP__