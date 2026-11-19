//==========================================
/// @file       amd64_interrupt.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_INTERRUPT_H
#define AMD64_INTERRUPT_H


#define AMD64_IDT_ENTRY_COUNT       256

#define AMD64_INT_IRQ_PIT           0x00
#define AMD64_INT_IRQ_PS2_KEYBOARD  0x01
#define AMD64_INT_IRQ_PS2_MOUSE     0x0c

#define AMD64_IDT_ATTR_PRESENT      (1u << 7)
#define AMD64_IDT_ATTR_RING0        (0u << 5)
#define AMD64_IDT_ATTR_RING3        (3u << 5)
#define AMD64_IDT_TYPE_INTERRUPT    0xeu
#define AMD64_IDT_TYPE_TRAP         0xfu

#include "libtype.h"
#include "libdef.h"

typedef struct __interrupt_frame {
    // (reverse of the push order)
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbp, rdi, rsi, rdx, rcx, rbx, rax;

    // error code or 0
    u64 vector;
    u64 code;

    // iret(q) frame
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __align_packed(16) interrupt_frame_t;

typedef struct __amd64_idt_entry {
    u16 offset_low16;
    u16 selector;
    u8 ist;
    u8 attributes;
    u16 offset_mid16;
    u32 offset_high32;
    u32 reserved;
} __packed amd64_idt_entry_t;

typedef struct __amd64_idtr {
    u16 limit;
    u64 base;
} __packed amd64_idtr_t;

typedef struct __amd64_idt {
    amd64_idt_entry_t entries[AMD64_IDT_ENTRY_COUNT];
} __align_packed(16) amd64_idt_t;

typedef interrupt_frame_t*(*interrupt_stub_t)(interrupt_frame_t* frame);

void amd64_cpu_idt_init();

#endif // AMD64_INTERRUPT_H