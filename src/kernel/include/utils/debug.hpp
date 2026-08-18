//==========================================
/// @file       debug.hpp
/// @brief      debug out stream impl
//==========================================

#pragma once

#ifndef __UTILS_DEBUG_HPP__
#define __UTILS_DEBUG_HPP__

#define DEBUG_ENABLED

#define AMD64_COM1 0x3F8

#define UART_DATA_PORT        0
#define UART_INTERRUPT_ENABLE 1
#define UART_FIFO_CTRL        2
#define UART_LINE_CTRL        3
#define UART_MODEM_CTRL       4
#define UART_LINE_STATUS      5

#define UART_TRANSMIT_READY         0x20
#define UART_DISABLE_INTERRUPTS     0x00
#define UART_ENABLE_DLAB            0x80
#define UART_DISABLE_DLAB           0x00
#define UART_BAUD_DIVISOR_LOW       0x03
#define UART_BAUD_DIVISOR_HIGH      0x00
#define UART_8N1                    0x03
#define UART_FIFO_ENABLE_CLEAR_14   0xC7
#define UART_MODEM_IRQ_RTS_DSR      0x0B

#define UART_CLOCK_HZ 115200
#define UART_BAUD_DIVISOR(baud) (UART_CLOCK_HZ / (baud))
#define UART_BAUD_115200        UART_BAUD_DIVISOR(115200)
#define UART_BAUD_57600         UART_BAUD_DIVISOR(57600)
#define UART_BAUD_38400         UART_BAUD_DIVISOR(38400)
#define UART_BAUD_19200         UART_BAUD_DIVISOR(19200)
#define UART_BAUD_14400         UART_BAUD_DIVISOR(14400)
#define UART_BAUD_9600          UART_BAUD_DIVISOR(9600)
#define UART_BAUD_4800          UART_BAUD_DIVISOR(4800)
#define UART_BAUD_2400          UART_BAUD_DIVISOR(2400)
#define UART_BAUD_1200          UART_BAUD_DIVISOR(1200)

#include "common.hpp"

#ifdef DEBUG_ENABLED
void debug_init();
void debug_putc(const char ch);
void debug_puts(const char* str);
void debug_trap(const char* str);
#else
#define debug_init()
#define debug_putc(x)
#define debug_puts(x, ...)
#define debug_trap(x)
#endif

void uart_init(u16 base, u16 divisor);
void uart_putc(u16 base, char ch);
void uart_puts(u16 base, const char* str);

#endif // __UTILS_DEBUG_HPP__