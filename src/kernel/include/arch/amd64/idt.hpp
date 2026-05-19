//==========================================
/// @file       idt.hpp
/// @brief      interrupt defines
//==========================================

#pragma once

#ifndef __AMD64_IDT_HPP__
#define __AMD64_IDT_HPP__

#define AMD64_INT_IDT_ENTRY_COUNT 256

#define AMD64_INT_PIC1            0x20
#define AMD64_INT_PIC1_DATA       0x21
#define AMD64_INT_PIC2            0xA0
#define AMD64_INT_PIC2_DATA       0xA1
#define AMD64_INT_PIC_EOI         0x20

#define AMD64_INT_IRQ_PIT             0x00
#define AMD64_INT_IRQ_PS2_KEYBOARD    0x01
#define AMD64_INT_IRQ_PS2_MOUSE       0x0C

#include "common.hpp"
#include "arch/amd64/cpu.hpp"

struct amd64_idt_entry_t {
    u16 isr_low;
    u16 kernel_cs;
    u8 ist;
    u8 attributes;
    u16 isr_mid;
    u32 isr_high;
    u32 reserved;
} PACKED;

struct amd64_idt_register_t {
    u16 limit;
    u64 base;
} PACKED;

typedef interrupt_regs_t*(*interrupt_dispatch_callback_t)(u64 code, interrupt_regs_t* stack);

void amd64_set_idt_entries(amd64_idt_entry_t* idt, u16 kernel_code_selector);
void amd64_set_idtr(amd64_idt_register_t* idtr, amd64_idt_entry_t* idt);
void amd64_set_interrupt_dispatch_callback(interrupt_dispatch_callback_t callback);
void amd64_interrupt_send_eoi(u8 irq);
void amd64_irq_unmask(u8 irq);

static inline
void amd64_call_scheduler_interrupt() {
    asm volatile("int $0x81");
}

static inline
void amd64_flush_idt(amd64_idt_register_t idtr) {
    asm volatile("lidt %0" : : "m"(idtr));
}

static inline
void amd64_interrupts_enable() {
    asm volatile ("sti");
}

#endif // __AMD64_IDT_HPP__