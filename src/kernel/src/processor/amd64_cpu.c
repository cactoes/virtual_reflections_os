#include "processor/amd64_linker.h"
#include "processor/amd64_cpu.h"
#include "libdef.h"

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
void cpuid(u32 leaf, u32 subleaf, u32* eax, u32* ebx, u32* ecx, u32* edx) {
    __asm__ volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

static
void enable_simd() {
    u64 new_cr0 = (read_cr0() | AMD64_CR0_MP) & ~(u64)AMD64_CR0_EM;
    write_cr0(new_cr0);
    u64 new_cr4 = read_cr4() | AMD64_CR4_OSFXSR | AMD64_CR4_OSXMMEXCPT;
    write_cr4(new_cr4);
}

// CLEAN @since 20/11/2026 -- 16:55
// move these msr functions

#define AMD64_EFER_NXE       (1ull << 11)
#define AMD64_MSR_EFER       0xc0000080u
#define AMD64_MSR_PAT        0x277u

static inline
void amd64_wrmsr(u32 addr, u64 value) {
    u32 low  = (u32)(value & 0xffffffff);
    u32 high = (u32)(value >> 32);

    __asm__ volatile ("wrmsr" :: "c"(addr), "a"(low), "d"(high) : "memory");
}

static inline
u64 amd64_rdmsr(u32 addr) {
    u32 low, high;

    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(addr) : "memory");

    return ((u64)high << 32) | low;
}

static
void enable_nxe() {
    amd64_wrmsr(AMD64_MSR_EFER, amd64_rdmsr(AMD64_MSR_EFER) | AMD64_EFER_NXE);
    amd64_wrmsr(AMD64_MSR_PAT, 0x0007040100070406ull);
}

void amd64_enable_processor_extensions(u64 extensions) {
    if (HAS_BIT(extensions, AMD64_CPU_EXTENSION_SIMD_BIT))
        enable_simd();
    
    if (HAS_BIT(extensions, AMD64_CPU_EXTENSION_NX_BIT))
        enable_nxe();
}

u64 amd64_get_processor_extensions() {
    u64 extensions = 0;
    u32 eax, ebx, ecx, edx;

    cpuid(AMD64_CPUID_LEAF_BASIC_MAX, 0, &eax, &ebx, &ecx, &edx);
    u32 max_basic = eax;

    cpuid(AMD64_CPUID_LEAF_EXTENDED_MAX, 0, &eax, &ebx, &ecx, &edx);
    u32 max_extended = eax;

    if (max_basic >= AMD64_CPUID_LEAF_FEATURES) {
        cpuid(AMD64_CPUID_LEAF_FEATURES, 0, &eax, &ebx, &ecx, &edx);
 
        if (HAS_ANY(edx, AMD64_CPUID1_EDX_SIMD) || HAS_ANY(ecx, AMD64_CPUID1_ECX_SIMD))
            extensions |= (1ull << AMD64_CPU_EXTENSION_SIMD_BIT);
    }

    if (max_extended >= AMD64_CPUID_LEAF_EXTENDED_INFO) {
        cpuid(AMD64_CPUID_LEAF_EXTENDED_INFO, 0, &eax, &ebx, &ecx, &edx);

        if (HAS_ANY(edx, AMD64_CPUID_EXT_NX))
            extensions |= (1ull << AMD64_CPU_EXTENSION_NX_BIT);
    }

// /* Enables NX if the CPU supports it. Without this, bit 63 is a reserved bit
//  * and every page with NX gives a page fault. Run on every core. */
// void amd64_nx_init(void) {
//     unsigned int a, b, c, d;
//     if (__get_cpuid(0x80000001, &a, &b, &c, &d) && (d & AMD64_CPUID_EXT_NX)) {
//         amd64_wrmsr(AMD64_MSR_EFER, amd64_rdmsr(AMD64_MSR_EFER) | AMD64_EFER_NXE);
//         g_nx_enabled = true;
//     }
// }

    return extensions;
}