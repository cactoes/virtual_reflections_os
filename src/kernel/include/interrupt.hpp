//==========================================
/// @file       interrupt.hpp
/// @brief      kernel interupts logic
//==========================================

#pragma once

#ifndef __INTERRUPT_HPP__
#define __INTERRUPT_HPP__

#define IDT_ENTRY_COUNT 256

#define PIC1            0x20
#define PIC1_DATA       0x21
#define PIC2            0xA0
#define PIC2_DATA       0xA1

#define INT_TYPE_COUNT          ((size_t)(interrupt_type::__ITEM_COUNT))
#define INT_TYPE_CAST(type)     ((uint64_t)(type))

#include "common.hpp"

struct cpu_state_t {
    uint64_t rflags;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbp;
    uint64_t rsp;
};

struct idt_entry_t {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_register_t {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

enum class interrupt_type {
    CRITICAL = 0,
    PIT,
    KEYBOARD,
    MOUSE,
    OTHER,
    __ITEM_COUNT
};

typedef cpu_state_t*(*interrupt_callback)(uint64_t code, cpu_state_t*);

extern "C" cpu_state_t* int_handler(uint64_t code, cpu_state_t* rsp);

void int_init();
void int_set_idt_entry(uint8_t vector, void* handler, uint8_t flags = 0x8E);
void int_set_callback(interrupt_type type, interrupt_callback int_cb);

#endif // __INTERRUPT_HPP__