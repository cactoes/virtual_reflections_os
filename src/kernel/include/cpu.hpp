//==========================================
/// @file       cpu.hpp
/// @brief      cpu functions
//==========================================

#pragma once

#ifndef __CPU_HPP__
#define __CPU_HPP__

#include "common.hpp"

// FIXME @since 15/05/2025 -- 18:58
// this struct is still in incorrect order
struct cpu_state_t {
    uint64_t rsp;
    uint64_t rflags;
    uint64_t cs;
    uint64_t rip;

    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;

    // uint64_t rip;
    // uint64_t cs;
    // uint64_t rflags;
    // uint64_t rsp;
} PACKED;

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

inline void* cpu_get_stack_pointer() {
    void* stack_pointer;
    __asm__ ("mov %%rsp, %0" : "=r" (stack_pointer));
    return stack_pointer;
}

#endif // __CPU_HPP__