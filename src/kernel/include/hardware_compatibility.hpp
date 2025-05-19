//==========================================
/// @file       hardware_compatibility.hpp
/// @brief      for making contact between software & hardware easier
///             or more standard
//==========================================

#pragma once

#ifndef __HARDWARE_COMPATIBILITY_HPP__
#define __HARDWARE_COMPATIBILITY_HPP__

//==========================================
/// @brief      interrupt defines
//==========================================
#define INT_IDT_ENTRY_COUNT 256

#define INT_PIC1            0x20
#define INT_PIC1_DATA       0x21
#define INT_PIC2            0xA0
#define INT_PIC2_DATA       0xA1
#define INT_PIC_EOI         0x20

#define INT_IRQ_PIT             0x00
#define INT_IRQ_PS2_KEYBOARD    0x01
#define INT_IRQ_PS2_MOUSE       0xC

#define INT_VECTOR_COUNT    48

//==========================================
/// @brief      gdt defines
//==========================================

#include "common.hpp"
#include "cpu.hpp"

/// @brief interrupt based compatibility stuff
namespace hc::interrupt {

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

/// @brief                  initializes interrupts & unmasks wanted irqs
/// @param[in] handler      pointer to interrupt handler
/// @param[in] irq_list     list of irqs to unmask
/// @param size             size of the list
/// @return                 0 succes, -1 invalid irq
int init(cpu_state_t*(*handler)(uint64_t code, cpu_state_t* rsp), uint8_t irq_list[], size_t size);

/// @brief                  creates an entry in the idt table
/// @param int_number       interrupt number to assing the handler to
/// @param[in] handler      pointer to the handler function
/// @return                 0 success
int set_idt_entry(uint8_t int_number, void* handler);

/// @brief              sends the end of interrupt signal to the PIC
/// @param irq_number   irq number
/// @return             0 success, -1 if irq was invalid
int pic_send_eoi(uint8_t irq_number);

/// @brief              unmasks an irq so the interrupt for it can be called
/// @param irq_number   irq number
/// @return             true success, false invalid irq
bool unmask_irq(uint8_t irq_number);

} // namespace hc::interrupt

/// @brief gdt based compatibility stuff
namespace hc::gdt {



} // namespace hc::gdt

#endif // __HARDWARE_COMPATIBILITY_HPP__