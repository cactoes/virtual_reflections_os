#include "processor/amd64_linker.h"
#include "processor/amd64_cpu.h"

static
u64 read_cr0() {
    u64 value;
    __asm__ volatile (
        "mov %%cr0, %0"
        : "=r"(value)
    );
    return value;
}
 
static
void write_cr0(u64 value) {
    __asm__ volatile (
        "mov %0, %%cr0"
        :
        : "r"(value)
        : "memory"
    );
}

static
u64 read_cr4() {
    u64 value;
    __asm__ volatile (
        "mov %%cr4, %0"
        :
        "=r"(value)
    );
    return value;
}
 
static
void write_cr4(u64 value) {
    __asm__ volatile (
        "mov %0, %%cr4"
        :
        : "r"(value)
        : "memory"
    );
}

static
void enable_simd() {
    u64 new_cr0 = (read_cr0() | AMD64_CR0_MP) & ~(u64)AMD64_CR0_EM;
    write_cr0(new_cr0);
    u64 new_cr4 = read_cr4() | AMD64_CR4_OSFXSR | AMD64_CR4_OSXMMEXCPT;
    write_cr4(new_cr4);
}

void amd64_enable_processor_extensions() {
    enable_simd();
}