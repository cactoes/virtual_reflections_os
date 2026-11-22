#include "processor/amd64_syscall.h"
#include "processor/amd64_gdt.h"
#include "processor/cpu_direct.h"
#include "processor/amd64_cpu.h"
#include "processor/amd64_msr.h"

// forward declare since its defined in the asm
void amd64_syscall_stub();

u64 amd64_syscall_dispatch(syscall_frame_t* frame) {
    // TODO @since 22/11/2026 -- 18:14
    // dispatch actuall syscall
    return 0;
}

void amd64_init_syscall(struct __cpu* cpu) {
    const u64 kernel_cs = AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_KERNEL_CODE, 0);
    const u64 sysret_base = AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_USER_CODE32, 3);
    const u64 star = (sysret_base << 48) | (kernel_cs << 32);

    amd64_msr_enable_sce();
    amd64_msr_set_star(star);
    amd64_msr_set_lstar((void*)amd64_syscall_stub);
    amd64_msr_set_sf_mask(AMD64_RFLAGS_TF | AMD64_RFLAGS_IF | AMD64_RFLAGS_DF | AMD64_RFLAGS_AC | AMD64_RFLAGS_NT);
    amd64_msr_set_gs_base(cpu);
    amd64_msr_set_kernel_gs_base(nullptr);
}