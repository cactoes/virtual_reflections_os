//==========================================
/// @file       debug.hpp
/// @brief      debug out stream impl
//==========================================

#pragma once

#ifndef __UTILS_DEBUG_HPP__
#define __UTILS_DEBUG_HPP__

#define DEBUG_ENABLED

#define DEBUG_PORT 0x3F8

#define DEBUG_SERIAL_DATA_PORT        0
#define DEBUG_SERIAL_INTERRUPT_ENABLE 1
#define DEBUG_SERIAL_FIFO_CTRL        2
#define DEBUG_SERIAL_LINE_CTRL        3
#define DEBUG_SERIAL_MODEM_CTRL       4
#define DEBUG_SERIAL_LINE_STATUS      5

#define DEBUG_SERIAL_TRANSMIT_READY         0x20
#define DEBUG_SERIAL_DISABLE_INTERRUPTS     0x00
#define DEBUG_SERIAL_ENABLE_DLAB            0x80
#define DEBUG_SERIAL_DISABLE_DLAB           0x00
#define DEBUG_SERIAL_BAUD_DIVISOR_LOW       0x03
#define DEBUG_SERIAL_BAUD_DIVISOR_HIGH      0x00
#define DEBUG_SERIAL_8N1                    0x03
#define DEBUG_SERIAL_FIFO_ENABLE_CLEAR_14   0xC7
#define DEBUG_SERIAL_MODEM_IRQ_RTS_DSR      0x0B

#include "common.hpp"

#ifdef DEBUG_ENABLED
void debug_init();
void debug_putc(const char ch);
void debug_puts(const char* p_str);
void debug_trap(const char* str);
#else
#define debug_init()
#define debug_putc(x)
#define debug_puts(x, ...)
#define debug_trap(x)
#endif

#endif // __UTILS_DEBUG_HPP__