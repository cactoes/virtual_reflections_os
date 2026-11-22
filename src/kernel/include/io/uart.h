//==========================================
/// @file       uart.h
/// @brief      
//==========================================

#pragma once

#ifndef UART_H
#define UART_H

#define UART_DATA_PORT        0
#define UART_INTERRUPT_ENABLE 1
#define UART_FIFO_CTRL        2
#define UART_LINE_CTRL        3
#define UART_MODEM_CTRL       4
#define UART_LINE_STATUS      5
#define UART_SCRATCH          7

#define UART_DIVISOR_LOW      UART_DATA_PORT
#define UART_DIVISOR_HIGH     UART_INTERRUPT_ENABLE

#define UART_DISABLE_INTERRUPTS 0x00

#define UART_LCR_WORD_8       (3u << 0)
#define UART_LCR_DLAB         (1u << 7)
#define UART_8N1              UART_LCR_WORD_8
#define UART_ENABLE_DLAB      UART_LCR_DLAB

#define UART_FCR_ENABLE       (1u << 0)
#define UART_FCR_CLEAR_RX     (1u << 1)
#define UART_FCR_CLEAR_TX     (1u << 2)
#define UART_FCR_TRIGGER_14   (3u << 6)
#define UART_FIFO_ENABLE_CLEAR_14 (UART_FCR_ENABLE | UART_FCR_CLEAR_RX | UART_FCR_CLEAR_TX | UART_FCR_TRIGGER_14)   // 0xC7

#define UART_MCR_DTR         (1u << 0)
#define UART_MCR_RTS         (1u << 1)
#define UART_MCR_OUT1        (1u << 2)
#define UART_MCR_OUT2        (1u << 3)
#define UART_MCR_LOOPBACK    (1u << 4)

#define UART_MCR_IRQ_ENABLED (UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2)         // 0x0B
#define UART_MCR_NORMAL      (UART_MCR_IRQ_ENABLED | UART_MCR_OUT1)
#define UART_MCR_LOOPTEST    (UART_MCR_RTS | UART_MCR_OUT1 | UART_MCR_OUT2 | UART_MCR_LOOPBACK)

#define UART_LSR_DATA_READY  (1u << 0)
#define UART_LSR_THR_EMPTY   (1u << 5)
#define UART_LSR_TX_IDLE     (1u << 6)
#define UART_TRANSMIT_READY  UART_LSR_THR_EMPTY

#define UART_TEST_BYTE_A     0xae
#define UART_TEST_BYTE_B     0x55

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

#include "libtype.h"

bool uart_init(u16 base, u16 divisor);
void uart_write_byte(u16 base, u8 byte);
void uart_write_string(u16 base, const char* str);

#endif // UART_H