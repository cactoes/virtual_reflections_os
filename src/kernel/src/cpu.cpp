#include "cpu.hpp"

static cpu_t cpu0 {};

cpu_t* get_current_cpu() {
    return &cpu0;
}

bool cpu_set_kernel_stack(cpu_t* cpu, void* stack) {
    if (!cpu || !stack)
        return false;

    cpu->kernel_rsp = (u64)stack;

    return true;
}