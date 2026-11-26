#include "processor/amd64_apic.h"
#include "processor/amd64_msr.h"
#include "memory/generic.h"
#include "processor/amd64_cpu.h"
#include "processor/amd64_interrupt.h"
#include "processor/amd64_pit.h"

static volatile u8* global_lapic;
static volatile u8* global_ioapic;
static u32 global_ioapic_gsi_count;
static madt_info_t global_madt_info;

static u32 global_lapic_timer_initial_count;
static volatile u64 global_lapic_timer_ticks[MAX_CPUS];

extern u64 cpu_count;

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

void amd64_lapic_enable_local() {
    u64 base = amd64_rdmsr(AMD64_MSR_LAPIC_BASE);
    amd64_wrmsr(AMD64_MSR_LAPIC_BASE, base | AMD64_MSR_LAPIC_ENABLE);

    lapic_write(global_lapic, AMD64_LAPIC_REG_TPR, 0);
    lapic_write(global_lapic, AMD64_LAPIC_REG_SVR, AMD64_LAPIC_SVR_ENABLE | AMD64_LAPIC_SPURIOUS_VECTOR);
}

void amd64_apic_init(madt_info_t* madt_info, cpu_t* cpus, u64 max_cpus) {
    global_madt_info = *madt_info;

    void* ioapic_base = (void*)(u64)global_madt_info.ioapics[0].address;

    u64 base_msr = amd64_rdmsr(AMD64_MSR_LAPIC_BASE);
    base_msr |= AMD64_MSR_LAPIC_ENABLE;
    amd64_wrmsr(AMD64_MSR_LAPIC_BASE, base_msr);

    const u64 lapic_phys = base_msr & 0x000ffffffffff000ull;
    global_lapic = (volatile u8*)kmmio_map((void*)lapic_phys, 0x1000);
    if (!global_lapic)
        return;

    amd64_lapic_enable_local();

    global_ioapic = (volatile u8*)kmmio_map(ioapic_base, 0x1000);
    if (!global_ioapic)
        return;

    global_ioapic_gsi_count = ((ioapic_read(global_ioapic, AMD64_IOAPIC_REG_VER) >> 16) & 0xff) + 1;

    for (u32 gsi = 0; gsi < global_ioapic_gsi_count; gsi++) {
        ioapic_write(global_ioapic, AMD64_IOAPIC_REG_REDIR(gsi) + 1, 0);
        ioapic_write(global_ioapic, AMD64_IOAPIC_REG_REDIR(gsi), AMD64_IOAPIC_MASKED);
    }

    u32 bsp_id = amd64_lapic_id();
    u64 next = 1;

    for (u64 i = 0; i < madt_info->cpu_count && i < max_cpus; i++) {
        u32 apic_id = madt_info->cpu_apic_id[i];
        if (apic_id == bsp_id)
            continue;

        cpus[next].id = next;
        cpus[next].lapic_id = apic_id;
        cpus[next].self = &cpus[next];
        next++;
    }
}

irq_route_t amd64_isa_irq_route(u32 isa_irq) {
    for (u32 i = 0; i < global_madt_info.override_count; i++) {
        if (global_madt_info.overrides[i].irq != isa_irq)
            continue;

        const u16 f = global_madt_info.overrides[i].flags;
        u32 flags = AMD64_IRQ_EDGE_HIGH;

        if (HAS_ALL(f, 3))
            flags |= AMD64_IRQ_POLARITY_LOW;
        if (HAS_ALL(f, 0xc))
            flags |= AMD64_IRQ_TRIGGER_LEVEL;

        return (irq_route_t){ global_madt_info.overrides[i].gsi, flags };
    }

    return (irq_route_t){ isa_irq, AMD64_IRQ_EDGE_HIGH };
}

static
bool lapic_wait_icr_idle() {
    for (u32 i = 0; i < 1000000u; i++) {
        if (!(lapic_read(global_lapic, 0x300) & (1u << 12)))
            return true;

        amd64_pause();
    }

    return false;
}

bool amd64_lapic_send_ipi(u32 target_apic_id, u32 command) {
    if (!global_lapic || target_apic_id > 255u)
        return false;

    if (amd64_rdmsr(AMD64_MSR_LAPIC_BASE) & (1ull << 10))
        return false;

    u64 flags = amd64_save_interrupts();
    bool ok = lapic_wait_icr_idle();

    if (ok) {
        lapic_write(global_lapic, 0x310, target_apic_id << 24);
        lapic_write(global_lapic, 0x300, command);
        ok = lapic_wait_icr_idle();
    }

    amd64_restore_interrupts(flags);
    return ok;
}

bool amd64_lapic_timer_init(u32 hz) {
    if (!global_lapic || hz == 0)
        return false;

    global_lapic_timer_initial_count = 0;

    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_LVT, AMD64_LAPIC_TIMER_VECTOR | AMD64_LAPIC_TIMER_MASKED);
    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_DIVIDE, AMD64_LAPIC_TIMER_DIVIDE_BY_16);
    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_INITIAL, 0xffffffffu);

    if (!pit_delay_us(AMD64_LAPIC_TIMER_CALIBRATION_US)) {
        lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_INITIAL, 0);
        return false;
    }

    const u32 remaining = lapic_read(global_lapic, AMD64_LAPIC_REG_TIMER_CURRENT);
    const u32 elapsed = 0xffffffffu - remaining;
    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_INITIAL, 0);

    if (elapsed == 0)
        return false;

    const u64 denominator = (u64)AMD64_LAPIC_TIMER_CALIBRATION_US * hz;
    const u64 numerator = (u64)elapsed * 1000000ull;
    const u64 count = (numerator + denominator / 2) / denominator;

    if (count == 0 || count > 0xffffffffull)
        return false;

    global_lapic_timer_initial_count = (u32)count;
    return amd64_lapic_timer_start_local();
}

bool amd64_lapic_timer_start_local() {
    if (!global_lapic || global_lapic_timer_initial_count == 0)
        return false;

    cpu_t* cpu = get_current_cpu();
    if (!cpu || cpu->id >= cpu_count)
        return false;

    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_LVT, AMD64_LAPIC_TIMER_VECTOR | AMD64_LAPIC_TIMER_PERIODIC | AMD64_LAPIC_TIMER_MASKED);
    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_DIVIDE, AMD64_LAPIC_TIMER_DIVIDE_BY_16);
    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_INITIAL, global_lapic_timer_initial_count);
    global_lapic_timer_ticks[cpu->id] = 0;

    lapic_write(global_lapic, AMD64_LAPIC_REG_TIMER_LVT, AMD64_LAPIC_TIMER_VECTOR | AMD64_LAPIC_TIMER_PERIODIC);
    return true;
}

void amd64_lapic_timer_handle_interrupt() {
    cpu_t* cpu = get_current_cpu();
    if (cpu && cpu->id < cpu_count)
        global_lapic_timer_ticks[cpu->id]++;

    amd64_lapic_eoi();
}

u64 amd64_lapic_timer_get_ticks() {
    cpu_t* cpu = get_current_cpu();
    if (!cpu || cpu->id >= MAX_CPUS)
        return 0;

    return global_lapic_timer_ticks[cpu->id];
}