//==========================================
/// @file       interrupt.hpp
/// @brief      x86_64 interrupt impl
//==========================================

#pragma once

#ifndef __X86_64_INTERRUPT_HPP__
#define __X86_64_INTERRUPT_HPP__

#define X86_64_INT_IDT_ENTRY_COUNT 256

#define X86_64_INT_PIC1            0x20
#define X86_64_INT_PIC1_DATA       0x21
#define X86_64_INT_PIC2            0xA0
#define X86_64_INT_PIC2_DATA       0xA1
#define X86_64_INT_PIC_EOI         0x20

#define X86_64_INT_IRQ_PIT             0x00
#define X86_64_INT_IRQ_PS2_KEYBOARD    0x01
#define X86_64_INT_IRQ_PS2_MOUSE       0x0C

#include "common.hpp"

struct x86_64_idt_entry_t {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} PACKED;

struct x86_64_idt_register_t {
    uint16_t limit;
    uint64_t base;
} PACKED;

void x86_64_set_idt_entries(x86_64_idt_entry_t* p_idt, uint16_t kernel_code_selector);
void x86_64_set_idtr(x86_64_idt_register_t* p_idtr, x86_64_idt_entry_t* p_idt);
void x86_64_set_handler(void*(p_handler)(uint64_t, void*));

static inline void x86_64_flush_idt(x86_64_idt_register_t idtr) {
    asm volatile("lidt %0" : : "m"(idtr));
}

static inline void x86_64_call_scheduler_interrupt() {
    asm volatile("int $0x81");
}

#endif // __X86_64_INTERRUPT_HPP__