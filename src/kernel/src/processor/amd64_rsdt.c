#include "processor/amd64_rsdt.h"
#include "memory/generic.h"
#include "libmem.h"

static
bool acpi_validate_checksum(const void* data, u32 length) {
    const u8* bytes = data;
    u8 sum = 0;
    for (u32 i = 0; i < length; i++)
        sum += bytes[i];
    return sum == 0;
}

static
acpi_sdt_header_t* acpi1_map_table(u64 paddr) {
    acpi_sdt_header_t* header = kmmio_map((void*)paddr, sizeof(acpi_sdt_header_t));
    if (!header)
        return nullptr;

    const u32 length = header->length;
    kmmio_unmap(header);

    if (length < sizeof(acpi_sdt_header_t) || length > AMD64_ACPI_SDT_MAX_LENGTH)
        return nullptr;

    header = kmmio_map((void*)paddr, length);
    if (!header)
        return nullptr;

    if (!acpi_validate_checksum(header, length)) {
        kmmio_unmap(header);
        return nullptr;
    }

    return header;
}

static
acpi_sdt_header_t* acpi1_find_table(const rsdt1_t* rsdt, const char* signature) {
    const u64 entries = (rsdt->header.length - sizeof(acpi_sdt_header_t)) / sizeof(u32);
    const u8* ptrs = (const u8*)rsdt->sdt_pointers;

    for (u64 i = 0; i < entries; i++) {
        u32 paddr32;
        memcpy(&paddr32, ptrs + i * sizeof(u32), sizeof(u32));

        acpi_sdt_header_t* table = acpi1_map_table(paddr32);
        if (!table)
            continue;

        if (memeq(table->signature, signature, 4))
            return table;

        kmmio_unmap(table);
    }

    return nullptr;
}

bool madt_parse(const madt_t* madt, madt_info_t* out) {
    memzero(out, sizeof(*out));
    out->lapic_base = madt->lapic_address;

    const u8* p   = (const u8*)madt + sizeof(madt_t);
    const u8* end = (const u8*)madt + madt->header.length;

    while (p + sizeof(madt_entry_header_t) <= end) {
        const madt_entry_header_t* e = (const madt_entry_header_t*)p;
        if (e->length < sizeof(*e) || p + e->length > end)
            break;

        switch (e->type) {
        case 0: {
            if (e->length < sizeof(madt_lapic_t)) break;
            const madt_lapic_t* c = (const madt_lapic_t*)p;
            const bool usable = (c->flags & 1) || (c->flags & 2);
            if (usable && out->cpu_count < AMD64_MADT_MAX_CPUS)
                out->cpu_apic_id[out->cpu_count++] = c->apic_id;
            break;
        }
        case 1: {
            if (e->length < sizeof(madt_ioapic_t)) break;
            const madt_ioapic_t* c = (const madt_ioapic_t*)p;
            if (out->ioapic_count < AMD64_MADT_MAX_IOAPICS) {
                u32 i = out->ioapic_count++;
                out->ioapics[i].id       = c->ioapic_id;
                out->ioapics[i].address  = c->address;
                out->ioapics[i].gsi_base = c->gsi_base;
            }
            break;
        }
        case 2: {
            if (e->length < sizeof(madt_iso_t)) break;
            const madt_iso_t* c = (const madt_iso_t*)p;
            if (c->bus == 0 && out->override_count < AMD64_MADT_MAX_OVERRIDES) {
                u32 i = out->override_count++;
                out->overrides[i].irq   = c->source;
                out->overrides[i].gsi   = c->gsi;
                out->overrides[i].flags = c->flags;
            }
            break;
        }
        case 5: {
            if (e->length < sizeof(madt_lapic_override_t)) break;
            out->lapic_base = ((const madt_lapic_override_t*)p)->address;
            break;
        }
        case 9: {
            if (e->length < sizeof(madt_x2apic_t)) break;
            const madt_x2apic_t* c = (const madt_x2apic_t*)p;
            if ((c->flags & 3) && out->cpu_count < AMD64_MADT_MAX_CPUS)
                out->cpu_apic_id[out->cpu_count++] = c->x2apic_id;
            break;
        }
        default:
            break;
        }

        p += e->length;
    }

    return out->cpu_count > 0 && out->ioapic_count > 0;
}

void amd64_rsdt1_init(void* paddr_rsdt, madt_info_t* madt_info) {
    rsdt1_t* rsdt = (rsdt1_t*)acpi1_map_table((u64)paddr_rsdt);
    if (!rsdt || !memeq(rsdt->header.signature, "RSDT", 4))
        return;

    acpi_sdt_header_t* madt = acpi1_find_table(rsdt, "APIC");
    if (!madt)
        return;

    madt_parse((madt_t*)madt, madt_info);
    return;
}