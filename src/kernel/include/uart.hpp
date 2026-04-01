//==========================================
/// @file       uart.h
/// @brief      
//==========================================

#pragma once

#ifndef __UART_HPP__
#define __UART_HPP__

#ifndef UART_TARGET
#define TARGET_QEMU
#endif

#ifdef TARGET_QEMU
    #define UART_TARGET
    #define UART_BASE       0x09000000UL
#elif TARGET_RPI4
    #define UART_TARGET
    #define UART_BASE       0xFE201000UL
#elif TARGET_RPI3
    #define UART_TARGET
    #define UART_BASE       0x3F201000UL
#else
    #error "no uart target"
#endif

#define UART_DR             0x000
#define UART_FR             0x018
#define UART_IBRD           0x024
#define UART_FBRD           0x028
#define UART_LCRH           0x02C
#define UART_CR             0x030
#define UART_IMSC           0x038
#define UART_PERIPH         0xFE0

#define UART_FR_TXFF        (1 << 5)
#define UART_FR_RXFE        (1 << 4)
#define UART_FR_BUSY        (1 << 3)

#define UART_LCRH_WLEN8     (3 << 5)
#define UART_LCRH_FEN       (1 << 4)

#define UART_CR_UARTEN      (1 << 0)
#define UART_CR_TXE         (1 << 8)
#define UART_CR_RXE         (1 << 9)

#define PL011_PERIPH_ID     0x11

#include "common.hpp"

bool uart_init();
void uart_putc(char c);
void uart_puts(const char* str);

#endif // __UART_HPP__