//==========================================
/// @file       debug.hpp
/// @brief      debug header file for debug func
//==========================================

#pragma once

#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__

#define SERIAL_PORT 0x3F8

#define SERIAL_DATA_PORT        0
#define SERIAL_INTERRUPT_ENABLE 1
#define SERIAL_FIFO_CTRL        2
#define SERIAL_LINE_CTRL        3
#define SERIAL_MODEM_CTRL       4
#define SERIAL_LINE_STATUS      5

#define SERIAL_TRANSMIT_READY         0x20
#define SERIAL_DISABLE_INTERRUPTS     0x00
#define SERIAL_ENABLE_DLAB            0x80
#define SERIAL_DISABLE_DLAB           0x00
#define SERIAL_BAUD_DIVISOR_LOW       0x03
#define SERIAL_BAUD_DIVISOR_HIGH      0x00
#define SERIAL_8N1                    0x03
#define SERIAL_FIFO_ENABLE_CLEAR_14   0xC7
#define SERIAL_MODEM_IRQ_RTS_DSR      0x0B

#define DEBUG_ENABLED

#include "common.hpp"

#ifdef DEBUG_ENABLED
void debug_init();
void debug_print(const char ch);
void debug_print(const char* fmt, ...);
#else
#define debug_init()
#define debug_print(x)
#define debug_print(x, ...)
#endif

#endif // __DEBUG_HPP__