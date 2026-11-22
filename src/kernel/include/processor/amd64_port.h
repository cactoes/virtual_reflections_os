//==========================================
/// @file       amd64_port.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_PORT_H
#define AMD64_PORT_H

#define AMD64_INT_PIC1              0x20
#define AMD64_INT_PIC1_DATA         0x21
#define AMD64_INT_PIC2              0xa0
#define AMD64_INT_PIC2_DATA         0xa1
#define AMD64_INT_PIC_EOI           0x20

#include "libtype.h"

static inline
void amd64_out_port8(u16 port, u8 value) {
    __asm__ volatile (
        "outb %1, %0"
        :
        : "dN"(port), "a"(value)
        : "memory"
    );
}

static inline
void amd64_out_port16(u16 port, u16 value) {
    __asm__ volatile (
        "outw %1, %0"
        :
        : "dN"(port), "a"(value)
        : "memory"
    );
}

static inline
void amd64_out_port32(u16 port, u32 value) {
    __asm__ volatile (
        "outl %1, %0"
        :
        : "dN"(port), "a"(value)
        : "memory"
    );
}

static inline
u8 amd64_in_port8(u16 port) {
    u8 value;
    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );
    return value;
}

static inline
u16 amd64_in_port16(u16 port) {
    u16 value;
    __asm__ volatile (
        "inw %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );
    return value;
}

static inline
u32 amd64_in_port32(u16 port) {
    u32 value;
    __asm__ volatile (
        "inl %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );
    return value;
}

static inline
void amd64_io_wait() {
    amd64_out_port8(0x80, 0);
}

#endif // AMD64_PORT_H