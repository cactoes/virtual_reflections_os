//==========================================
/// @file       mouse.hpp
/// @brief      ps2 mouse driver
//==========================================

#pragma once

#ifndef __DRIVERS_PS2_MOUSE_HPP__
#define __DRIVERS_PS2_MOUSE_HPP__

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

#define PS2_MOUSE_STATUS_MB_LEFT        0x01
#define PS2_MOUSE_STATUS_MB_RIGHT       0x02
#define PS2_MOUSE_STATUS_MB_MIDDLE      0x04
#define PS2_MOUSE_STATUS_VALID_PKT      0x08
#define PS2_MOUSE_STATUS_X_SIGN_BIT     0x10
#define PS2_MOUSE_STATUS_Y_SIGN_BIT     0x20
#define PS2_MOUSE_STATUS_X_OVERFLOW     0x40
#define PS2_MOUSE_STATUS_Y_OVERFLOW     0x80

#define PS2_MOUSE_PACKET_SIZE   4

#define PS2_MOUSE_STATUS_OVERFLOW_MASK  (PS2_MOUSE_STATUS_X_OVERFLOW | PS2_MOUSE_STATUS_Y_OVERFLOW)
#define PS2_MOUSE_STATUS_SIGN_MASK      (PS2_MOUSE_STATUS_X_SIGN_BIT | PS2_MOUSE_STATUS_Y_SIGN_BIT)

#define PS2_MOUSE_SMALL_MOVEMENT_THRESH     0x80

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
} __packed;

void ps2_mouse_init();
void* ps2_mouse_handle_interrupt(void* stack, void*);
void ps2_mouse_event_subscribe(void(*p_handler)(const ps2_mouse_state_t*));
const ps2_mouse_state_t* ps2_mouse_get_state();
void ps2_mouse_process_packet();

#endif // __DRIVERS_PS2_MOUSE_HPP__