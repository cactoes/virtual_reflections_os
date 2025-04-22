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

#define PIC_EOI         0x20

#define IRQ_PIT         0x00
#define IRQ_KEYBOARD    0x01
#define IRQ_PS2_MOUSE   0x12

#define INT_TYPE_COUNT          ((size_t)(interrupt_type::__ITEM_COUNT))
#define INT_TYPE_CAST(type)     ((uint64_t)(type))

#include "common.hpp"
#include "cpu.hpp"

struct idt_entry_t {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} PACKED;

struct idt_register_t {
    uint16_t limit;
    uint64_t base;
} PACKED;

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
void int_pic_send_eoi(uint8_t irq);

#endif // __INTERRUPT_HPP__