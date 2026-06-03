//==========================================
/// @file       apic.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AMD64_APIC_HPP__
#define __AMD64_APIC_HPP__

#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

#define AMD64_IOAPIC_DEFAULT_BASE       0xFEC00000

#define AMD64_APIC_TPR                  0x80
#define AMD64_APIC_SVR                  0xF0
#define AMD64_APIC_EOI                  0xB0
#define AMD64_APIC_TPR_ACCEPT_ALL       0x00

#define AMD64_IOAPIC_ID                 0x00
#define AMD64_IOAPIC_ENTRIES            0x10

void amd64_init_apic();

#endif

#endif // __AMD64_APIC_HPP__