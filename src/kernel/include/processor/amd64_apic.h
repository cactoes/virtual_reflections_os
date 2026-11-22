//==========================================
/// @file       amd64_apic.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_APIC_H
#define AMD64_APIC_H

#define AMD64_LAPIC_REG_ID            0x020
#define AMD64_LAPIC_REG_TPR           0x080
#define AMD64_LAPIC_REG_EOI           0x0b0
#define AMD64_LAPIC_REG_SVR           0x0f0
#define AMD64_LAPIC_SVR_ENABLE        (1u << 8)

#define AMD64_IOAPIC_REGSEL           0x00
#define AMD64_IOAPIC_WIN              0x10
#define AMD64_IOAPIC_REG_VER          0x01
#define AMD64_IOAPIC_REG_REDIR(n)     (0x10 + (n) * 2)
#define AMD64_IOAPIC_MASKED           (1u << 16)
#define AMD64_IOAPIC_DEFAULT_BASE     0xfec00000ull

#define AMD64_IRQ_POLARITY_LOW   (1u << 13)
#define AMD64_IRQ_TRIGGER_LEVEL  (1u << 15)
#define AMD64_IRQ_EDGE_HIGH      0u
#define AMD64_IRQ_LEVEL_LOW      (AMD64_IRQ_TRIGGER_LEVEL | AMD64_IRQ_POLARITY_LOW)

#define AMD64_LAPIC_SPURIOUS_VECTOR 0xff

typedef struct __irq_route_t {
    u32 gsi;
    u32 flags;
} irq_route_t;

#include "libtype.h"
#include "amd64_rsdt.h"

void amd64_apic_init(madt_info_t* madt_info);
u32 amd64_lapic_id();
void amd64_lapic_eoi();

bool amd64_ioapic_route(u32 gsi, u8 vector, u32 flags);
bool amd64_ioapic_mask(u32 gsi, bool masked);

irq_route_t amd64_isa_irq_route(u32 isa_irq);

#endif // AMD64_APIC_H