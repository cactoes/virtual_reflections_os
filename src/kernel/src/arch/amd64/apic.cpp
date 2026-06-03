#include "arch/amd64/apic.hpp"
#include "arch/amd64/msr.hpp"
#include "arch/amd64/idt.hpp"
#include "arch/amd64/port.hpp"

#include "memory/vmem.hpp"

#include "io.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

void amd64_ioapic_write(void* ioapic_base, u8 reg, u32 val) {
    *(volatile u32*)((u8*)ioapic_base + AMD64_IOAPIC_ID) = reg;
    *(volatile u32*)((u8*)ioapic_base + AMD64_IOAPIC_ENTRIES) = val;
}

u32 amd64_ioapic_read(void* ioapic_base, u8 reg) {
    *(volatile u32*)((u8*)ioapic_base + AMD64_IOAPIC_ID) = reg;
    return *(volatile u32*)((u8*)ioapic_base + AMD64_IOAPIC_ENTRIES);
}

void amd64_ioapic_route_irq(void* ioapic_base, u8 irq, u8 vector, u8 lapic_id) {
    u32 low = vector;
    u32 high = (lapic_id << 24);
    amd64_ioapic_write(ioapic_base, 0x10 + irq * 2, low);
    amd64_ioapic_write(ioapic_base, 0x10 + irq * 2 + 1, high);
}

void* global_lapic = nullptr;

void amd64_init_apic() {
    amd64_interrupts_disable();

    amd64_out_port8(AMD64_INT_PIC1_DATA, 0xff);
    amd64_out_port8(AMD64_INT_PIC2_DATA, 0xff);

    u64 lapic_base_msr = amd64_rdmsr(AMD64_MSR_LAPIC_BASE);
    lapic_base_msr |= AMD64_MSR_LAPIC_ENABLE; // Set APIC Global Enable bit
    amd64_wrmsr(AMD64_MSR_LAPIC_BASE, lapic_base_msr);

    u64 lapic_phys = lapic_base_msr & ~0xFFFull;
    global_lapic = vmem_map_mmio_region((void*)lapic_phys);

    *((volatile u32*)((u8*)global_lapic + AMD64_APIC_TPR)) = AMD64_APIC_TPR_ACCEPT_ALL;
    *((volatile u32*)((u8*)global_lapic + AMD64_APIC_SVR)) = 0x1FF;

    void* ioapic_base = vmem_map_mmio_region((void*)AMD64_IOAPIC_DEFAULT_BASE);

    amd64_ioapic_route_irq(ioapic_base, 2,  0x20, 0); // PIT
    amd64_ioapic_route_irq(ioapic_base, 1,  0x21, 0); // PS/2 Keyboard
    amd64_ioapic_route_irq(ioapic_base, 12, 0x2C, 0); // PS/2 Mouse

    // FIXME @since 03/06/2026 -- 23:46
    // quick hack to fix the qemu mapping
    amd64_ioapic_route_irq(ioapic_base, 11, 0x2B, 0); // HARDWARE_FFP_SSCI_NIC2

    amd64_interrupts_enable();
}

#endif