#include "processor/amd64_ap.h"
#include "processor/cpu_direct.h"
#include "processor/amd64_linker.h"
#include "processor/amd64_vmm.h"
#include "processor/amd64_msr.h"
#include "processor/amd64_port.h"
#include "processor/amd64_gdt.h"
#include "processor/amd64_apic.h"
#include "processor/amd64_interrupt.h"
#include "processor/amd64_syscall.h"
#include "processor/amd64_cpu.h"
#include "processor/amd64_pit.h"
#include "memory/generic.h"
#include "io/debug.h"
#include "libatomic.h"
#include "libmem.h"

static ap_boot_data_t* ap_boot_data;
extern cpu_t cpus[MAX_CPUS];

// forward declare
void virtual_core_entry(cpu_t* core);

void amd64_ap_entry(cpu_t* cpu, u64 extensions) {
    amd64_disable_interrupts();

    amd64_enable_processor_extensions(extensions);
    amd64_cpu_gdt_init(cpu->id, cpu->kernel_rsp);
    amd64_cpu_idt_load();
    amd64_lapic_enable_local();
    amd64_init_syscall(cpu);
    amd64_lapic_timer_start_local();

    atomic_store32(&cpu->ipi_ping_count, 0);
    atomic_store32(&cpu->online, 1);

    while (!atomic_fetch32(&cpu->start))
        __asm__ volatile ("sti; hlt" ::: "memory");

    virtual_core_entry(cpu);
}

bool amd64_ap_prepare(kboot_info_t* kboot_info) {
    bool usable = false;

    for (u64 i = 0; i < kboot_info->memory.region_count; i++) {
        const memory_region_t* region = &kboot_info->memory.regions[i];
        if ((region->addr & MEMORY_TYPE_MASK) != MEMORY_USABLE)
            continue;

        const u64 start = region->addr & ~MEMORY_TYPE_MASK;
        if (start <= AP_TRAMPOLINE_PHYS && region->size >= (AP_TRAMPOLINE_PHYS - start) + AP_PAGE_SIZE) {
            usable = true;
            break;
        }
    }

    if (!usable)
        return false;

    u64 kernel_cr3 = amd64_read_cr3();

    const u64 kernel_phys = kernel_cr3 & 0x000ffffffffff000ull;
    const u64 boot_phys = (u64)amd64_ap_boot_pml4;

    if (kernel_phys >= 0x40000000ull || boot_phys >= 0x40000000ull)
        return false;

    const u64* kernel_pml4 = (const u64*)KERNEL_PHYS_TO_VIRT(kernel_phys);
    u64* boot_pml4 = (u64*)KERNEL_PHYS_TO_VIRT(boot_phys);
    if (!(kernel_pml4[AMD64_PML4_KERNEL_INDEX] & 1ull))
        return false;

    memcpy(boot_pml4, kernel_pml4, AP_PAGE_SIZE);
    boot_pml4[0] = kernel_pml4[AMD64_PML4_KERNEL_INDEX];
    boot_pml4[AMD64_RECURSIVE_SLOT] = boot_phys | 3ull;

    const u64 image_size = (u64)amd64_ap_trampoline_end - (u64)amd64_ap_trampoline_start;
    if (image_size != AP_DATA_OFFSET + sizeof(ap_boot_data_t))
        return false;

    void* image = (void*)KERNEL_PHYS_TO_VIRT(AP_TRAMPOLINE_PHYS);
    memcpy(image, amd64_ap_trampoline_start, image_size);

    ap_boot_data = (ap_boot_data_t*)((u64)image + AP_DATA_OFFSET);
    ap_boot_data->boot_cr3 = boot_phys;
    ap_boot_data->kernel_cr3 = kernel_cr3;
    ap_boot_data->entry = (u64)amd64_ap_entry;
    ap_boot_data->extensions = cpu_get_processor_extensions();
    ap_boot_data->pat = amd64_rdmsr(AMD64_MSR_PAT);

    return true;
}

static
bool smp_start_one_ap(cpu_t* cpu) {
    const u32 vector = (u32)(AP_TRAMPOLINE_PHYS >> 12);

    atomic_store32(&cpu->online, 0);
    ap_boot_data->cpu = (u64)cpu;
    ap_boot_data->stack_top = cpu->kernel_rsp;

    amd64_membarier();

    if (!amd64_lapic_send_ipi(cpu->lapic_id, SMP_IPI_INIT_ASSERT))
        return false;

    if (!pit_delay_us(10000u))
        return false;

    if (!amd64_lapic_send_ipi(cpu->lapic_id, SMP_IPI_INIT_DEASSERT))
        return false;

    if (!amd64_lapic_send_ipi(cpu->lapic_id, SMP_IPI_STARTUP | vector))
        return false;

    if (!pit_delay_us(200u))
        return false;

    if (!amd64_lapic_send_ipi(cpu->lapic_id, SMP_IPI_STARTUP | vector))
        return false;

    if (!pit_delay_us(200u))
        return false;

    for (u32 ms = 0; ms < SMP_AP_TIMEOUT_MS; ms++) {
        if (atomic_fetch32(&cpu->online))
            return true;

        if (!pit_delay_us(1000u))
            return false;
    }

    return atomic_fetch32(&cpu->online) != 0;
}

static
bool ping_ap(u64 cpu_index) {
    cpu_t* cpu = &cpus[cpu_index];

    if (!atomic_fetch32(&cpu->online))
        return false;
    
    const u32 before = atomic_fetch32(&cpu->ipi_ping_count);
    
    if (!amd64_lapic_send_ipi(cpu->lapic_id, AMD64_IPI_PING_VECTOR))
        return false;

    bool acknowledged = false;

    for (u32 ms = 0; ms < 1000u; ms++) {
        const u32 current = atomic_fetch32(&cpu->ipi_ping_count);
        if (current != before) {
            acknowledged = true;
            break;
        }

        if (!pit_delay_us(1000u))
            return false;
    }

    if (!acknowledged)
        acknowledged = atomic_fetch32(&cpu->ipi_ping_count) != before;

    return acknowledged;
}

bool amd64_ap_start_and_ping(u64 cpu_index) {
    if (!ap_boot_data || !ap_boot_data->boot_cr3 || !ap_boot_data->kernel_cr3 || !ap_boot_data->entry)
        return false;

    const u32 bsp_apic_id = amd64_lapic_id();
    cpu_t* cpu = &cpus[cpu_index];
    if (cpu->self != cpu || cpu->id != (u32)cpu_index || !cpu->kernel_rsp || (cpu->kernel_rsp & 15ull) || cpu->lapic_id > 255u || cpu->lapic_id == bsp_apic_id)
        return false;

    if (!smp_start_one_ap(cpu))
        return false;

    for (u64 i = 0; i < 2; i++) {
        if (!ping_ap(cpu_index))
            return false;
    }

    const u32 count = atomic_fetch32(&cpus[cpu_index].ipi_ping_count);
    return count == 2;
}

void amd64_ap_startup(kboot_info_t* kboot_info, u64 cpu_count) {
    if (!amd64_ap_prepare(kboot_info)) {
        kdbg_printf("failed to prepair ap trampoline");
        return;
    }

    for (u64 i = 1; i < cpu_count; i++) {
        u64 base = (u64)kalloc(CPU_KERNEL_STACK_SIZE);
        if (!base) {
            kdbg_printf("failed to allocate kernel stack: cpu(%u)\n", i);
            return;
        }

        cpus[i].kernel_rsp = align_down(base + CPU_KERNEL_STACK_SIZE, 16);

        if (!amd64_ap_start_and_ping(i)) {
            kdbg_printf("failed to start: cpu(%u)\n", i);
            continue;
        }
    }
}

bool amd64_ap_send_ipi(u64 cpu_index, u32 vector) {
    cpu_t* cpu = &cpus[cpu_index];
    if (!cpu)
        return false;

    if (!atomic_fetch32(&cpu->online))
        return false;

    if (!amd64_lapic_send_ipi(cpu->lapic_id, vector))
        return false;

    return true;
}
