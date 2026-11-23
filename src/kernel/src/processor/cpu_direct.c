#include "processor/cpu_direct.h"
#include "io/debug.h"
#include "memory/generic.h"
#include "libmem.h"

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
#include "processor/amd64_cpu.h"
#include "processor/amd64_interrupt.h"
#include "processor/amd64_gdt.h"
#include "processor/amd64_apic.h"
#include "processor/amd64_syscall.h"
#include "processor/amd64_linker.h"
#include "processor/amd64_rsdt.h"
#include "processor/amd64_msr.h"
#else
#error "undefined cpu architecture";
#endif

static cpu_interrupt_handler_t handlers[256];
static cpu_t cpus[MAX_CPUS];

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

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64

#define AP_TRAMPOLINE_PHYS 0x8000ull
#define AP_DATA_OFFSET     0x300ull
#define AP_PAGE_SIZE       0x1000ull
#define AP_PML4_KERNEL_INDEX ((KERNEL_VIRTUAL_BASE >> 39) & 0x1ffull)
#define AP_PML4_RECURSIVE_INDEX 511

extern char amd64_ap_trampoline_start[];
extern char amd64_ap_trampoline_end[];
extern u64 amd64_ap_boot_pml4[];

void amd64_ap_entry(cpu_t* cpu, u64 extensions);

typedef struct {
    u64 boot_cr3;
    u64 kernel_cr3;
    u64 stack_top;
    u64 cpu;
    u64 entry;
    u64 extensions;
    u64 pat;
} ap_boot_data_t;

static ap_boot_data_t* ap_boot_data;

static bool prepare_ap_trampoline(const kboot_info_t* boot_info) {
    bool usable = false;
    for (u64 i = 0; i < boot_info->memory.region_count; i++) {
        const memory_region_t* region = &boot_info->memory.regions[i];
        if ((region->addr & MEMORY_TYPE_MASK) != MEMORY_USABLE)
            continue;

        const u64 start = region->addr & ~MEMORY_TYPE_MASK;
        if (start <= AP_TRAMPOLINE_PHYS && region->size >= AP_PAGE_SIZE &&
            AP_TRAMPOLINE_PHYS - start <= region->size - AP_PAGE_SIZE) {
            usable = true;
            break;
        }
    }
    if (!usable)
        return false;

    u64 kernel_cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(kernel_cr3));
    const u64 kernel_phys = kernel_cr3 & 0x000ffffffffff000ull;
    const u64 boot_phys = (u64)amd64_ap_boot_pml4;

    // The current boot direct map covers the first 1 GB only.
    if (kernel_phys >= 0x40000000ull || boot_phys >= 0x40000000ull)
        return false;

    const u64* kernel_pml4 = (const u64*)KERNEL_PHYS_TO_VIRT(kernel_phys);
    u64* boot_pml4 = (u64*)KERNEL_PHYS_TO_VIRT(boot_phys);
    if (!(kernel_pml4[AP_PML4_KERNEL_INDEX] & 1ull))
        return false;

    memcpy(boot_pml4, kernel_pml4, AP_PAGE_SIZE);
    boot_pml4[0] = kernel_pml4[AP_PML4_KERNEL_INDEX];
    boot_pml4[AP_PML4_RECURSIVE_INDEX] = boot_phys | 3ull;

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

void amd64_ap_entry(cpu_t* cpu, u64 extensions) {
    amd64_disable_interrupts();

    amd64_enable_processor_extensions(extensions);
    amd64_cpu_gdt_init(cpu->id, cpu->kernel_rsp);
    amd64_cpu_idt_load();
    amd64_lapic_enable_local();
    amd64_init_syscall(cpu);

    // TODO @since 23/11/2026 -- 21:28
    // atomic exchange
    cpu->online = 1;

    // park core until ipi/scheduling
    for (;;)
        __asm__ volatile ("cli; hlt" ::: "memory");
}

#endif

void boot_cpu_init(kboot_info_t* kboot_info) {
#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    amd64_disable_interrupts();
    amd64_cpu_gdt_init(0, KERNEL_PHYS_TO_VIRT(KERNEL_STACK_TOP));
    amd64_cpu_idt_init();

    // on x86-64 extra value 0 is the rsdt table point (paddr)
    madt_info_t madt_info;
    amd64_rsdt1_init(kboot_info->extra_values[0], &madt_info);

    kdbg_printf("system cores: %u\n", madt_info.cpu_count);

    u64 cpu_count = madt_info.cpu_count < MAX_CPUS ? madt_info.cpu_count : MAX_CPUS;

    amd64_apic_init(&madt_info, cpus, cpu_count);

    cpu_t* base_cpu = &cpus[0];
    base_cpu->id = 0;
    base_cpu->lapic_id = amd64_lapic_id();
    base_cpu->kernel_rsp = KERNEL_PHYS_TO_VIRT(KERNEL_STACK_TOP);
    base_cpu->self = base_cpu;
    base_cpu->online = 1;

    amd64_init_syscall(base_cpu);

    for (u64 i = 1; i < cpu_count; i++) {
        u64 base = (u64)kalloc(CPU_KERNEL_STACK_SIZE);
        if (!base) {
            kdbg_printf("failed to allocate kernel stack: cpu(%u)", i);
            return;
        }

        cpus[i].kernel_rsp = align_down(base + CPU_KERNEL_STACK_SIZE, 16);
    }

    if (cpu_count > 1) {
        if (!prepare_ap_trampoline(kboot_info)) {
            kdbg_printf("failed to prepare AP trampoline\n");
            for (;;) {
                __asm__ volatile ("cli; hlt" ::: "memory");
            }
        }

        kdbg_printf("AP trampoline prepared\n");
    }

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