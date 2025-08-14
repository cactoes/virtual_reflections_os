//==========================================
/// @file       interrupt.hpp
/// @brief      interrupt implementation
//==========================================

#pragma once

#ifndef __INTERRUPT_HPP__
#define __INTERRUPT_HPP__

#define ARCH_X86_64
#ifdef ARCH_X86_64
#include "arch/x86_64/interrupt.hpp"

#define INT_IRQ_PIT             X86_64_INT_IRQ_PIT
#define INT_IRQ_PS2_KEYBOARD    X86_64_INT_IRQ_PS2_KEYBOARD
#define INT_IRQ_PS2_MOUSE       X86_64_INT_IRQ_PS2_MOUSE

void interrupt_init(uint16_t kernel_code_selector);
void interrupt_set_handler(void*(p_handler)(uint64_t, cpu_state_t*));
void interrupt_send_eoi(uint8_t irq_num);

static inline void call_scheduler_interrupt() {
    x86_64_call_scheduler_interrupt();
}

#endif // ARCH_X86_64

#endif // __INTERRUPT_HPP__