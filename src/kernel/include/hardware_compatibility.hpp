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
#define GDT_ACCESS_PRESENT      (1 << 7)
#define GDT_ACCESS_RING0        (0 << 0)
// #define GDT_ACCESS_RING1        0b00100000
// #define GDT_ACCESS_RING2        0b01000000
#define GDT_ACCESS_RING3        ((1 << 5) | (1 << 6))
#define GDT_ACCESS_SEGMENT      (1 << 4)
#define GDT_ACCESS_EXECUTABLE   (1 << 3)
#define GDT_ACCESS_DIRECTION    (1 << 2)
#define GDT_ACCESS_READWRITE    (1 << 1)
#define GDT_ACCESS_ACCESSED     (1 << 0)

#define GDT_GRANULARITY_4K      (1 << 7)
#define GDT_GRANULARITY_1B      (0 << 0)
#define GDT_OPERAND_SIZE_32     (1 << 6)
#define GDT_LONG_MODE           (1 << 5)
#define GDT_AVAILABLE           (1 << 4)

#define GDT_INDEX_NULL                  0
#define GDT_INDEX_KERNEL_64_CODE        1
#define GDT_INDEX_TSS                   sizeof(hc::gdt_tss::gdt_t::entries) / sizeof(hc::gdt_tss::gdt_entry_t)

#define GDT_INDEX_TO_ENTRY(index)       (index) * sizeof(hc::gdt_tss::gdt_entry_t)

//==========================================
/// @brief      pit defines
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
/// @return                 0 succes, 1 invalid irq
int init(cpu_state_t*(*handler)(uint64_t code, cpu_state_t* rsp), uint8_t irq_list[], size_t size);

/// @brief                  creates an entry in the idt table
/// @param int_number       interrupt number to assing the handler to
/// @param[in] handler      pointer to the handler function
/// @return                 0 success
int set_idt_entry(uint8_t int_number, void* handler);

/// @brief              sends the end of interrupt signal to the PIC
/// @param irq_number   irq number
/// @return             0 success, 1 if irq was invalid
int pic_send_eoi(uint8_t irq_number);

/// @brief              unmasks an irq so the interrupt for it can be called
/// @param irq_number   irq number
/// @return             true success, false invalid irq
bool unmask_irq(uint8_t irq_number);

} // namespace hc::interrupt

/// @brief gdt based compatibility stuff
namespace hc::gdt_tss {

struct gdt_entry_t {
    uint16_t limit;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high8;
};

struct tss_entry_t {
    uint16_t length;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t flags1;
    uint8_t flags2;
    uint8_t base_high8;
    uint32_t base_upper32;
    uint32_t reserved;
};

struct gdt_t {
    gdt_entry_t entries[2];
    tss_entry_t tss_entry;
};

struct gdtr_t {
    uint16_t limit;
    uint64_t address;
} PACKED;

struct tss_t {
    uint32_t resereved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t resereved1;
    uint64_t resereved2;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t resereved3;
    uint16_t resereved4;
    uint16_t iomap_offset;
} PACKED;

int init();
int set_stack_pointer0(void* stack_pointer);
uint64_t get_kernel_code_selector();

} // namespace hc::gdt_tss

/// @brief pit based compatibility stuff
namespace hc::pit {

int init(uint64_t times_per_s);
uint64_t read();

} // namespace hc::pit

#endif // __HARDWARE_COMPATIBILITY_HPP__