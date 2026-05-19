#include "cpu.hpp"
#include "arch/msr.hpp"
#include "arch/gdt.hpp"

extern "C" void x86_64_syscall_handler();

static cpu_t cpu0 {};

bool initialize_cpus() {
    // for now we only have a single core
    
    // uint64_t star = ((uint64_t)(USER_CODE_32_SELECTOR_INDEX << 3) << 48)
    //               | ((uint64_t)(KERNEL_CODE_SELECTOR_INDEX << 3) << 32);

    // msr_enable_sce();
    // msr_set_star(star);
    // msr_set_lstar((void*)x86_64_syscall_handler);
    // msr_set_sf_mask(RFLAGS_TF | RFLAGS_IF | RFLAGS_DF);
    // msr_set_gs_base(&cpu0);
    // msr_set_kernel_gs_base(0);

    // TODO @since 23/04/2026 -- 21:12
    // add the gdt etc here

    return true;
}

cpu_t* get_current_cpu() {
    return &cpu0;
}

bool set_kernel_stack(cpu_t* cpu, void* stack) {
    if (!cpu || !stack)
        return false;

    cpu->kernel_rsp = (uint64_t)stack;

    return true;
}