#include "processor/amd64_apic.h"
#include "processor/amd64_msr.h"
#include "memory/generic.h"

static volatile u8* global_lapic;
static volatile u8* global_ioapic;
static u32 global_ioapic_gsi_count;

static
u32 lapic_read(volatile u8* lapic, u32 reg) {
    return *(volatile u32*)(lapic + reg);
}

static
void lapic_write(volatile u8* lapic, u32 reg, u32 val) {
    *(volatile u32*)(lapic + reg) = val;
}

static
u32 ioapic_read(volatile u8* ioapic, u32 reg) {
    *(volatile u32*)(ioapic + AMD64_IOAPIC_REGSEL) = reg;
    return *(volatile u32*)(ioapic + AMD64_IOAPIC_WIN);
}

static
void ioapic_write(volatile u8* ioapic, u32 reg, u32 val) {
    *(volatile u32*)(ioapic + AMD64_IOAPIC_REGSEL) = reg;
    *(volatile u32*)(ioapic + AMD64_IOAPIC_WIN) = val;
}

u32 amd64_lapic_id() {
    return lapic_read(global_lapic, AMD64_LAPIC_REG_ID) >> 24;
}

void amd64_lapic_eoi() {
    lapic_write(global_lapic, AMD64_LAPIC_REG_EOI, 0);
}

bool amd64_ioapic_route(u32 gsi, u8 vector, u32 flags) {
    if (gsi >= global_ioapic_gsi_count)
        return false;

    const u32 reg = AMD64_IOAPIC_REG_REDIR(gsi);
    ioapic_write(global_ioapic, reg + 1, amd64_lapic_id() << 24);
    ioapic_write(global_ioapic, reg, vector | flags);
    return true;
}

bool amd64_ioapic_mask(u32 gsi, bool masked) {
    if (gsi >= global_ioapic_gsi_count)
        return false;

    const u32 reg = AMD64_IOAPIC_REG_REDIR(gsi);
    u32 low = ioapic_read(global_ioapic, reg);
    low = masked ? (low | AMD64_IOAPIC_MASKED) : (low & ~AMD64_IOAPIC_MASKED);
    ioapic_write(global_ioapic, reg, low);
    return true;
}

void amd64_apic_init() {
    // TODO @since 22/11/2026 -- 16:43
    // read acpi madt
    void* ioapic_base = (void*)AMD64_IOAPIC_DEFAULT_BASE;

    u64 base_msr = amd64_rdmsr(AMD64_MSR_LAPIC_BASE);
    base_msr |= AMD64_MSR_LAPIC_ENABLE;
    amd64_wrmsr(AMD64_MSR_LAPIC_BASE, base_msr);

    const u64 lapic_phys = base_msr & 0x000ffffffffff000ull;
    global_lapic = (volatile u8*)kmmio_map((void*)lapic_phys, 0x1000);
    if (!global_lapic)
        return;

    lapic_write(global_lapic, AMD64_LAPIC_REG_TPR, 0);
    lapic_write(global_lapic, AMD64_LAPIC_REG_SVR, AMD64_LAPIC_SVR_ENABLE | AMD64_LAPIC_SPURIOUS_VECTOR);

    global_ioapic = (volatile u8*)kmmio_map(ioapic_base, 0x1000);
    if (!global_ioapic)
        return;

    global_ioapic_gsi_count = ((ioapic_read(global_ioapic, AMD64_IOAPIC_REG_VER) >> 16) & 0xff) + 1;

    for (u32 gsi = 0; gsi < global_ioapic_gsi_count; gsi++) {
        ioapic_write(global_ioapic, AMD64_IOAPIC_REG_REDIR(gsi) + 1, 0);
        ioapic_write(global_ioapic, AMD64_IOAPIC_REG_REDIR(gsi), AMD64_IOAPIC_MASKED);
    }
}