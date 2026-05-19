//==========================================
/// @file       port.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AMD64_PORT_HPP__
#define __AMD64_PORT_HPP__

#include "common.hpp"

static inline
u8 amd64_in_port8(u16 port) {
    u8 value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline
u16 amd64_in_port16(u16 port) {
    u16 value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline
u32 amd64_in_port32(u16 port) {
    u32 value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline
void amd64_out_port8(u16 port, u8 value) {
    asm volatile ("outb %1, %0" : : "dN"(port), "a"(value));
}

static inline
void amd64_out_port16(u16 port, u16 value) {
    asm volatile ("outw %1, %0" : : "dN"(port), "a"(value));
}

static inline
void amd64_out_port32(u16 port, u32 value) {
    asm volatile ("outl %1, %0" : : "dN"(port), "a"(value));
}

#endif // __AMD64_PORT_HPP__