//==========================================
/// @file       amd64_rsdt.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_RSDT_H
#define AMD64_RSDT_H

#define AMD64_MADT_MAX_CPUS             256
#define AMD64_MADT_MAX_IOAPICS          8
#define AMD64_MADT_MAX_OVERRIDES        16

#define AMD64_ACPI_SDT_MAX_LENGTH       (16u * 1024 * 1024)

#include "libtype.h"
#include "libdef.h"

typedef struct __acpi_sdt_header_t {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __packed acpi_sdt_header_t;

typedef struct __rsdt {
    acpi_sdt_header_t header;
    u32 sdt_pointers[];
} __packed rsdt1_t;

typedef struct __madt {
    acpi_sdt_header_t header;
    u32 lapic_address;
    u32 flags;
} __packed madt_t;

typedef struct __madt_entry_header {
    u8 type;
    u8 length;
} __packed madt_entry_header_t;

typedef struct __madt_lapic {
    madt_entry_header_t header;
    u8 acpi_processor_id;
    u8 apic_id;
    u32 flags;
} __packed madt_lapic_t;

typedef struct __madt_ioapic {
    madt_entry_header_t header;
    u8 ioapic_id;
    u8 reserved;
    u32 address;
    u32 gsi_base;
} __packed madt_ioapic_t;

typedef struct __madt_iso {
    madt_entry_header_t header;
    u8 bus;
    u8 source;
    u32 gsi;
    u16 flags;
} __packed madt_iso_t;

typedef struct __madt_lapic_override {
    madt_entry_header_t header;
    u16 reserved;
    u64 address;
} __packed madt_lapic_override_t;

typedef struct __madt_x2apic {
    madt_entry_header_t header;
    u16 reserved;
    u32 x2apic_id;
    u32 flags;
    u32 acpi_id;
} __packed madt_x2apic_t;

typedef struct __madt_info {
    u64 lapic_base;

    u32 cpu_count;
    u32 cpu_apic_id[AMD64_MADT_MAX_CPUS];

    u32 ioapic_count;
    struct {
        u8 id;
        u32 address;
        u32 gsi_base;
    } ioapics[AMD64_MADT_MAX_IOAPICS];

    u32 override_count;
    struct {
        u8 irq;
        u32 gsi;
        u16 flags;
    } overrides[AMD64_MADT_MAX_OVERRIDES];
} madt_info_t;

void amd64_rsdt1_init(void* paddr_rsdt, madt_info_t* madt_info);

#endif // AMD64_RSDT_H