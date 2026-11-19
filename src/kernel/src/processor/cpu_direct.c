#include "processor/cpu_direct.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_cpu.h"
#include "processor/amd64_interrupt.h"
#include "processor/amd64_gdt.h"
#else
#error "undefined cpu architecture";
#endif

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
    // msr ?
    amd64_enable_interrupts();
#else
#endif
}