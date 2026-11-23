#include "processor/cpu_direct.h"
#include "io/debug.h"
#include "memory/generic.h"
#include "libmem.h"
#include "libatomic.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_cpu.h"
#include "processor/amd64_interrupt.h"
#include "processor/amd64_gdt.h"
#include "processor/amd64_apic.h"
#include "processor/amd64_syscall.h"
#include "processor/amd64_linker.h"
#include "processor/amd64_rsdt.h"
#include "processor/amd64_msr.h"
#include "processor/amd64_port.h"
#include "processor/amd64_ap.h"
#include "processor/amd64_tlb.h"
#else
#error "undefined cpu architecture";
#endif

static cpu_interrupt_handler_t handlers[256];
cpu_t cpus[MAX_CPUS];
u64 cpu_count;

u64 cpu_get_processor_extensions() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    return amd64_get_processor_extensions();
#else
    return 0;
#endif
}

void cpu_enable_processor_extensions(u64 extensions) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    amd64_enable_processor_extensions(extensions);
#else
#endif
}

void boot_cpu_init(kboot_info_t* kboot_info) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    amd64_disable_interrupts();
    amd64_cpu_gdt_init(0, KERNEL_PHYS_TO_VIRT(KERNEL_STACK_TOP));
    amd64_cpu_idt_init();

    // on x86-64 extra value 0 is the rsdt table point (paddr)
    madt_info_t madt_info;
    amd64_rsdt1_init(kboot_info->extra_values[0], &madt_info);

    kdbg_printf("system cores: %u\n", madt_info.cpu_count);

    cpu_count = madt_info.cpu_count < MAX_CPUS ? madt_info.cpu_count : MAX_CPUS;

    amd64_apic_init(&madt_info, cpus, cpu_count);

    cpu_t* base_cpu = &cpus[0];
    base_cpu->id = 0;
    base_cpu->lapic_id = amd64_lapic_id();
    base_cpu->kernel_rsp = KERNEL_PHYS_TO_VIRT(KERNEL_STACK_TOP);
    base_cpu->self = base_cpu;
    base_cpu->online = 1;

    amd64_init_syscall(base_cpu);

    amd64_tlb_init();
    amd64_enable_interrupts();

    if (cpu_count > 1)
        amd64_ap_startup(kboot_info, cpu_count);

    amd64_tlb_invalidate_page(base_cpu);
    amd64_tlb_flush_all();
#else
#endif
}

bool cpu_bind_interrupt(u32 line, u32 flags, cpu_interrupt_handler_t handler) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    if (line >= AMD64_IRQ_LINE_LIMIT)
        return false;

    u32 hw_flags = 0;
    if (flags & CPU_IRQ_TRIGGER_LEVEL)
        hw_flags |= AMD64_IRQ_TRIGGER_LEVEL;

    if (flags & CPU_IRQ_POLARITY_LOW)
        hw_flags |= AMD64_IRQ_POLARITY_LOW;

    handlers[line] = handler;

    return amd64_ioapic_route(line, AMD64_IRQ_VECTOR_BASE + line, hw_flags);
#else
    return false;
#endif
}

bool cpu_unbind_interrupt(u32 line) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    if (line >= AMD64_IRQ_LINE_LIMIT)
        return false;

    handlers[line] = nullptr;

    return amd64_ioapic_mask(line, true);
#else
    return false;
#endif
}

cpu_interrupt_handler_t cpu_get_interrupt_handler(u32 line) {
    return handlers[line];
}

cpu_t* get_current_cpu() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    cpu_t* cpu;
    __asm__ volatile ("mov %%gs:16, %0" : "=r"(cpu));
    return cpu;
#else
    return nullptr;
#endif
}

void cpu_enable_all_cores() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    for (u64 i = 1; i < cpu_count; i++)
        amd64_am_send_ipi(i, AMD64_IPI_START_VECTOR);
#else
#endif
}