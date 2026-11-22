//==========================================
/// @file       amd64_uart.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_UART_H
#define AMD64_UART_H

#define AMD64_COM1_BASE     0x3f8
#define AMD64_COM2_BASE     0x2f8
#define AMD64_COM3_BASE     0x3e8
#define AMD64_COM4_BASE     0x2e8

#include "libtype.h"

bool amd64_uart_init(u16 base, u16 divisor);
void amd64_uart_write_byte(u16 base, u8 byte);

#endif // AMD64_UART_H