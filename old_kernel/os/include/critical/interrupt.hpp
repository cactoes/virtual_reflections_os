//==========================================
/// @file       interrupt.hpp
/// @brief      kernel interupts logic
//==========================================

#pragma once

#ifndef __INTERRUPT_HPP___
#define __INTERRUPT_HPP__

#include "common.hpp"

#define IDT_ENTRY_COUNT 256

#define PIC1            0x20
#define PIC1_DATA       0x21
#define PIC2            0xA0
#define PIC2_DATA       0xA1

typedef struct __CPU_STATE {
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
} cpu_state_t;

/// @brief          general interupt handler
/// @param code     interupt number
/// @param[in] rsp  current stack pointer
/// @returns        new stack pointer
extern "C" cpu_state_t*
kernel_interrupt_handler(
    uint32_t code,
    cpu_state_t* rsp);

/// @brief namespace for interrupts
namespace kernel::interrupt {

typedef struct __INTERUPT_DESCRIPTOR_TABLE_ENTRY {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct __INTERUPT_DESCRIPTOR_TABLE_REGISTER {
    uint16_t    limit;
    uint64_t    base;
} __attribute__((packed)) idt_register_t;

/// @brief initialize idt
void
init();

/// @brief              sets an idt entry at a vector index
/// @param vector       index of the function (idt index)
/// @param[in] handler  pointer to the handler function
/// @param flags        additional flags to pass
void
set_idt_entry(
    uint8_t vector,
    void* handler,
    uint8_t flags = 0x8E);

} // namespace kernel::interrupt

#endif // __INTERRUPT_HPP__