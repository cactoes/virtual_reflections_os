//==========================================
/// @file       interrupt_manager.hpp
/// @brief      all logic for interrupts on a higher level
//==========================================

#pragma once

#ifndef __INTERRUPT_MANAGER_HPP__
#define __INTERRUPT_MANAGER_HPP__

#include "common.hpp"

enum class interrupt_t : u64 {
    EXCEPTION_DIVISION_BY_ZERO = 0,
    EXCEPTION_SINGLE_STEP_INTERRUPT = 1,
    EXCEPTION_NMI = 2,
    EXCEPTION_BREAKPOINT = 3,
    EXCEPTION_OVERFLOW = 4,
    EXCEPTION_BOUND_RANGE_EXCEEDED = 5,
    EXCEPTION_INVALID_OPCODE = 6,
    EXCEPTION_COPROCESSOR_NOT_AVAILABLE = 7,
    EXCEPTION_DOUBLE_FAULT = 8,
    EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN = 9,
    EXCEPTION_INVALID_TSS = 10,
    EXCEPTION_SEGMENT_NOT_PRESENT = 11,
    EXCEPTION_STACK_SEGMENT_FAULT = 12,
    EXCEPTION_GENERAL_PROTECTION_FAULT = 13,
    EXCEPTION_PAGE_FAULT = 14,
    EXCEPTION_RESERVED = 15,
    EXCEPTION_X87_FLOATING_POINT_EXCEPTION = 16,
    EXCEPTION_ALIGNMENT_CHECK = 17,
    EXCEPTION_MACHINE_CHECK = 18,
    EXCEPTION_SIMD_FP_EXCEPTION = 19,
    EXCEPTION_VIRTUALIZATION_EXCEPTION = 20,
    EXCEPTION_CONTROL_PROTECTION_EXCEPTION = 21,

    HARDWARE_PIT = 22,
    HARDWARE_KEYBOARD = 23,
    HARDWARE_CASCADE = 24,
    HARDWARE_COM2 = 25,
    HARDWARE_COM1 = 26,
    HARDWARE_LPT2 = 27,
    HARDWARE_FLOPPY_DISK = 28,
    HARDWARE_LPT1 = 29,
    HARDWARE_CMOS_RTC = 30,
    HARDWARE_FFP_L_SCSI_NIC = 31,
    HARDWARE_FFP_SSCI_NIC1 = 32,
    HARDWARE_FFP_SSCI_NIC2 = 33,
    HARDWARE_PS2_MOUSE = 34,
    HARDWARE_COPROCESSOR = 35,
    HARDWARE_PRIMARY_ATA_HD = 36,
    HARDWARE_SECONDARY_ATA_HD = 37,

    SOFTWARE_SCHEDULER,

    SIZE,

    UNKOWN = (u64)-1,
};

typedef void*(*interrupt_callback_t)(void* stack, void* data);

struct interrupt_hook_t {
    interrupt_callback_t callback;
    void* data;
};

void* interrupt_manager_dispatch(interrupt_t interrupt, void* stack);
bool hook_interrupt(interrupt_t code, interrupt_callback_t callback, void* data);

/// @brief          checks if current section is in side an interrupt
/// @return         is in interrupt state
bool is_in_interrupt();

void disable_interrupts();
void enable_interrupts();

#endif // __INTERRUPT_MANAGER_HPP__