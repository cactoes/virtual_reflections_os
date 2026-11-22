#include "processor/cpu_direct.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_cpu.h"
#include "processor/amd64_interrupt.h"
#include "processor/amd64_gdt.h"
#include "processor/amd64_apic.h"
#include "processor/amd64_syscall.h"
#include "processor/amd64_linker.h"
#else
#error "undefined cpu architecture";
#endif

static cpu_interrupt_handler_t handlers[256];
static cpu_t current_cpu;

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

void cpu_init() {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    amd64_disable_interrupts();
    amd64_cpu_gdt_init();
    amd64_cpu_idt_init();
    amd64_apic_init();

    current_cpu.kernel_rsp = KERNEL_STACK_TOP;
    current_cpu.id = amd64_lapic_id();
    current_cpu.self = &current_cpu;
    amd64_init_syscall(&current_cpu);
    amd64_enable_interrupts();
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