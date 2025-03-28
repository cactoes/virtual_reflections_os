//==========================================
/// @file       cpu.hpp
/// @brief      cpu functions
//==========================================

#pragma once

#ifndef __CPU_HPP__
#define __CPU_HPP__

#include "common.hpp"

inline void cpu_outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" : : "dN"(port), "a"(value));
}

inline uint8_t cpu_inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

inline void cpu_outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1, %0" : : "dN"(port), "a"(value));
}

inline uint16_t cpu_inw(uint16_t port) {
    uint16_t value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

inline void cpu_outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

inline uint32_t cpu_inl(uint16_t port) {
    uint32_t value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

[[noreturn]] inline void cpu_halt() {
    for (;;) {
        asm volatile ("cli");
        asm volatile ("hlt");
    }
}

#endif // __CPU_HPP__