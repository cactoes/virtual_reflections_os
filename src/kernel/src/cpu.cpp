#include "cpu.hpp"
#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64
#include "arch/amd64/cpu.hpp"
#endif

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

bool cpu_get_name(char* buffer, u64 size) {
#if CPU_ARCHITECTURE == ARCH_AMD64
    if (!buffer || size < 48)
        return false;

    u32 registers[4];
    char* p = buffer;
    for (u32 i = 0; i < 3; i++) {
        amd64_cpuid(0x80000002 + i, 0, &registers[0], &registers[1], &registers[2], &registers[3]);
        memcpy(p, registers, sizeof(registers));
        p += sizeof(registers);
    }

    return true;
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
}