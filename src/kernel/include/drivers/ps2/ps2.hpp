//==========================================
/// @file       ps2.hpp
/// @brief      ps2 generic driver stuff
///  TODO       expand / rework the ps2 device driver
//==========================================

#pragma once

#ifndef __DRIVERS_PS2_PS2_HPP__
#define __DRIVERS_PS2_PS2_HPP__

#define PS2_DATA_PORT   0x60
#define PS2_CMD_PORT    0x64

#define PS2_CTRL_STATUS_OUT_BUF     0x01
#define PS2_CTRL_STATUS_IN_BUF      0x02
#define PS2_CTRL_STATUS_SYSTEM      0x04
#define PS2_CTRL_STATUS_CMD_DATA    0x08
#define PS2_CTRL_STATUS_LOCKED      0x10
#define PS2_CTRL_STATUS_AUX_BUF     0x20
#define PS2_CTRL_STATUS_TIMEOUT     0x40
#define PS2_CTRL_STATUS_PARITY      0x80

#define PS2_CMD_TEST_FIRST_PORT     0xAB
#define PS2_CMD_TEST_SECOND_PORT    0xA9
#define PS2_CMD_DISABLE_FIRST_PORT  0xAD
#define PS2_CMD_DISABLE_SECOND_PORT 0xA7
#define PS2_CMD_ENABLE_FIRST_PORT   0xAE
#define PS2_CMD_ENABLE_SECOND_PORT  0xA8

#define PS2_TEST_PASSED             0x00
#define PS2_TEST_FAILED             0xFF

#define PS2_DEVICE_DISABLE_SCANNING 0xF5
#define PS2_DEVICE_ENABLE_SCANNING  0xF4
#define PS2_DEVICE_IDENTIFY         0xF2
#define PS2_DEVICE_RESET            0xFF

#define PS2_DEVICE_ID_MOUSE         0x00
#define PS2_DEVICE_ID_WHEEL_MOUSE   0x03
#define PS2_DEVICE_ID_5BTN_MOUSE    0x04
#define PS2_DEVICE_ACK              0xFA
#define PS2_DEVICE_RESEND           0xFE
#define PS2_DEVICE_ERROR            0xFC

#include "common.hpp"

enum class ps2_device_type_t {
    KEYBOARD = 0,
    MOUSE = 1
};

void ps2_wait_input();
void ps2_wait_output();

void ps2_write(u8 port, u8 value);
u8 ps2_read(u8 port);
bool ps2_port_test_device(ps2_device_type_t device_type);

#endif // __DRIVERS_PS2_PS2_HPP__