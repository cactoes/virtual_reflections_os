//==========================================
/// @file       mouse_driver.hpp
/// @brief      mouse driver implementation
//==========================================

#pragma once

#ifndef __MOUSE_DRIVER_HPP__
#define __MOUSE_DRIVER_HPP__

#define PS2_CMD_PORT                0x64
#define PS2_DATA_PORT               0x60

#define PS2_ENABLE_AUX_DEVICE       0xA8
#define PS2_READ_CMD_BYTE           0x20
#define PS2_WRITE_CMD_BYTE          0x60
#define PS2_WRITE_TO_MOUSE          0xD4

#define PS2_MOUSE_SET_DEFAULTS      0xF6
#define PS2_MOUSE_ENABLE_REPORTING  0xF4
#define PS2_CMD_SET_SAMPLE_RATE     0xF3

#define PS2_CMD_ENABLE_MOUSE_IRQ    0x02

#define PS2_SAMPLE_RATE_1    200
#define PS2_SAMPLE_RATE_2    100
#define PS2_SAMPLE_RATE_3     80

#include "interrupt.hpp"

struct mouse_state_t {
    int dx;
    int dy;
    int ds;

    struct {
        bool left : 1;
        bool middle : 1;
        bool right : 1;
    } buttons;
} __attribute__((packed));

void ps2_mouse_init(mouse_state_t* mouse_state);

cpu_state_t* mouse_handle_interrupt(uint64_t code, cpu_state_t* rsp);

#endif // __MOUSE_DRIVER_HPP__