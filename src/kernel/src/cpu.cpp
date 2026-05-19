#include "cpu.hpp"

extern "C" void x86_64_syscall_handler();

static cpu_t cpu0 {};

bool initialize_cpus() {
    return true;
}

cpu_t* get_current_cpu() {
    return &cpu0;
}

bool set_kernel_stack(cpu_t* cpu, void* stack) {
    if (!cpu || !stack)
        return false;

    cpu->kernel_rsp = (u64)stack;

    return true;
}