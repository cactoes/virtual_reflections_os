#include "processor/amd64_tlb.h"
#include "processor/amd64_cpu.h"
#include "processor/amd64_apic.h"
#include "processor/amd64_interrupt.h"
#include "processor/cpu_direct.h"
#include "io/debug.h"
#include "libatomic.h"
#include "libdef.h"

extern cpu_t cpus[MAX_CPUS];

static u32 tlb_ready;
static u32 tlb_request_lock;
static u32 tlb_pending[MAX_CPUS];
static const void* tlb_request_address;
static bool tlb_request_full;

__noreturn
static
void tlb_fatal(const char* reason, u32 cpu_id) {
    // TODO @since 23/11/2026 -- 23:38
    // proper crash handler1

    kdbg_uprintf("tlb shootdown fatal: %s (cpu %u)\n", reason, cpu_id);
    while (true)
        __asm__ volatile ("cli; hlt" ::: "memory");
}

static
void tlb_invalidate_local(const void* address, bool full) {
    if (!full) {
        amd64_flush_tlb(address);
        return;
    }

    const u64 interrupt_flags = amd64_save_interrupts();
    const u64 cr4 = amd64_read_cr4();
    if (cr4 & TLB_CR4_PCIDE)
        tlb_fatal("no pcid", 0);

    if (cr4 & TLB_CR4_PGE)
        amd64_write_cr4(cr4 & ~TLB_CR4_PGE);
    else
        amd64_reload_cr3();

    amd64_restore_interrupts(interrupt_flags);
}

static
void tlb_lock_requests() {
    u64 flags = amd64_read_rflags();
    if (!(flags & AMD64_RFLAGS_IF))
        tlb_fatal("requires IF=1", 0);

    while (true) {
        u32 busy = atomic_xchg32(&tlb_request_lock, 1);
        if (!busy)
            return;

        while (atomic_fetch32(&tlb_request_lock))
            amd64_pause();
    }
}

void amd64_tlb_init() {
    if (amd64_read_cr4() & TLB_CR4_PCIDE)
        tlb_fatal("no pcid", 0);

    atomic_store32(&tlb_ready, 1);
}

static
u32 tlb_shootdown(const void* address, bool full) {
    if (!atomic_fetch32(&tlb_ready)) {
        tlb_invalidate_local(address, full);
        return 0;
    }

    tlb_lock_requests();
    cpu_t* current = get_current_cpu();
    bool targets[MAX_CPUS] = { false };
    u32 target_count = 0;

    tlb_request_address = address;
    tlb_request_full = full;

    for (u32 i = 0; i < MAX_CPUS; i++) {
        if (&cpus[i] == current || !atomic_fetch32(&cpus[i].online))
            continue;

        targets[i] = true;
        target_count++;
        atomic_store32(&tlb_pending[i], 1);
    }

    amd64_membarier();

    tlb_invalidate_local(address, full);

    for (u32 i = 0; i < MAX_CPUS; i++) {
        if (targets[i] && !amd64_lapic_send_ipi(cpus[i].lapic_id, AMD64_IPI_TLB_VECTOR))
            tlb_fatal("failed to send ipi", i);
    }

    for (u32 i = 0; i < MAX_CPUS; i++) {
        if (!targets[i])
            continue;

        u32 polls = 0;
        while (atomic_fetch32(&tlb_pending[i])) {
            if (++polls == TLB_ACK_POLL_LIMIT)
                tlb_fatal("acknowledgement failed", i);

            amd64_pause();
        }
    }

    atomic_store32(&tlb_request_lock, 0);
    return target_count;
}

u32 amd64_tlb_invalidate_page(const void* address) {
    return tlb_shootdown(address, false);
}

u32 amd64_tlb_flush_all() {
    return tlb_shootdown(nullptr, true);
}

void amd64_tlb_handle_ipi(void) {
    cpu_t* cpu = get_current_cpu();
    if (cpu->id >= MAX_CPUS)
        tlb_fatal("invalid cpu id", cpu->id);

    if (atomic_fetch32(&tlb_pending[cpu->id])) {
        tlb_invalidate_local(tlb_request_address, tlb_request_full);
        amd64_lapic_eoi();
        atomic_store32(&tlb_pending[cpu->id], 0);
        return;
    }

    amd64_lapic_eoi();
}