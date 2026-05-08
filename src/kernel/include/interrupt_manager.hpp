//==========================================
/// @file       interrupt_manager.hpp
/// @brief      all logic for interrupts on a higher level
//==========================================

#pragma once

#ifndef __INTERRUPT_MANAGER_HPP__
#define __INTERRUPT_MANAGER_HPP__

#include "common.hpp"
#include "cpu.hpp"

typedef interrupt_regs_t*(*interrupt_callback_t)(interrupt_regs_t*, void*);

struct interrupt_hook_t {
    interrupt_callback_t callback;
    void* data;
};

enum class interrupt_t : uint64_t {
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

    SOFTWARE_SYSTEMCALL,
    SOFTWARE_SCHEDULER,
    // TODO @since 20/08/2025 -- 02:15
    SOFTWARE_CRASH_HANDLER,

    SIZE,

    UNKOWN = (uint64_t)-1,
};

/// @brief              checks if the given interrupt code is an exception
/// @param code         interrupt code to check
/// @return             true if code is in the exception range (0-21), false otherwise
bool is_interrupt_exception(uint64_t code);

/// @brief              checks if the given interrupt code is an exception
/// @param code         interrupt code to check
/// @return             true if code is in the exception range (0-21), false otherwise
bool is_interrupt_exception(interrupt_t code);

/// @brief              checks if the given interrupt code is a hardware interrupt
/// @param code         interrupt code to check
/// @return             true if code is a hardware interrupt, false otherwise
bool is_interrupt_hardware(uint64_t code);

/// @brief              checks if the given interrupt code is a hardware interrupt
/// @param code         interrupt code to check
/// @return             true if code is a hardware interrupt, false otherwise
bool is_interrupt_hardware(interrupt_t code);

/// @brief              converts a raw interrupt code to an interrupt_t enum value
/// @param code         raw interrupt code to convert
/// @return             interrupt_t
interrupt_t convert_to_interrupt(uint64_t code);

bool set_interrupt_hook(interrupt_t code, interrupt_callback_t callback, void* data);

/// @brief          convert a hardware irq number to its corresponding interrupt vector
/// @param irq      hardware irq number to convert
/// @return         interrupt vector number corresponding to the given irq
uint64_t interrupt_irq_to_int(uint64_t irq);

/// @brief          handles an interrupt by dispatching to the correct callback or handler
/// @param code     interrupt code to handle
/// @param[in]      p_rsp   pointer to cpu state at time of interrupt
/// @return         updated cpu state pointer after handling the interrupt
void* handle_interrupt(uint64_t code, interrupt_regs_t* p_rsp);

/// @brief          checks if current section is in side an interrupt
/// @return         is in interrupt state
bool is_in_interrupt();

#endif // __INTERRUPT_MANAGER_HPP__