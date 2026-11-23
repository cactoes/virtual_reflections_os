#include "libmem.h"
#include "processor/amd64_gdt.h"
#include "processor/amd64_linker.h"
#include "processor/cpu_direct.h"

static amd64_gdt_t gdts[MAX_CPUS];
static amd64_gdtr_t gdtrs[MAX_CPUS];
static amd64_tss_t tsses[MAX_CPUS];

static const amd64_gdt_entry_t gdt_zero_entry = {
    .limit_low16 = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = 0,
    .limit_high4_flags = 0,
    .base_high8 = 0,
};

static const amd64_gdt_entry_t gdt_kernel_code_entry = {
    .limit_low16 = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING0 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_EXECUTABLE |
              AMD64_GDT_ACCESS_READWRITE,
    .limit_high4_flags = AMD64_GDT_LONG_MODE,
    .base_high8 = 0,
};

static const amd64_gdt_entry_t gdt_kernel_data_entry = {
    .limit_low16 = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING0 | 
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_READWRITE,
    .limit_high4_flags = 0, 
    .base_high8 = 0,
};

static const amd64_gdt_entry_t gdt_user_code32_entry = {
    .limit_low16 = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING3 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_EXECUTABLE |
              AMD64_GDT_ACCESS_READWRITE,
    .limit_high4_flags = 0, // not set since we dont have a 32 compatibility mode
    .base_high8 = 0,
};

static const amd64_gdt_entry_t gdt_user_data_entry = {
    .limit_low16 = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING3 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_READWRITE,
    .limit_high4_flags = 0,
    .base_high8 = 0,
};

static const amd64_gdt_entry_t gdt_user_code_entry = {
    .limit_low16 = 0,
    .base_low16 = 0,
    .base_mid8 = 0,
    .access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_ACCESS_RING3 |
              AMD64_GDT_ACCESS_SEGMENT | AMD64_GDT_ACCESS_EXECUTABLE |
              AMD64_GDT_ACCESS_READWRITE,
    .limit_high4_flags = AMD64_GDT_LONG_MODE,
    .base_high8 = 0,
};

static
bool gdt_set_entry(amd64_gdt_t* gdt, const amd64_gdt_entry_t* entry, u64 index) {
    if (index >= GDT_SEGMENT_COUNT)
        return false;

    memcpy(&gdt->entries[index], entry, sizeof(amd64_gdt_entry_t));
    return true;
}

static
void set_tss(amd64_gdt_t* gdt, const amd64_tss_t* tss) {
    u64 base = (u64)tss;

    gdt->tss.limit_low16 = sizeof(amd64_tss_t) - 1;
    gdt->tss.base_low16 = (u16)base;
    gdt->tss.base_mid8 = (u8)(base >> 16);
    gdt->tss.access = AMD64_GDT_ACCESS_PRESENT | AMD64_GDT_TYPE_TSS64_AVAILABLE;
    gdt->tss.limit_high4_flags = 0;
    gdt->tss.base_high8 = (u8)(base >> 24);
    gdt->tss.base_upper32 = (u32)(base >> 32);
    gdt->tss.reserved = 0;
}

static
void update_gdtr(amd64_gdtr_t* gdtr, const amd64_gdt_t* gdt) {
    gdtr->limit = sizeof(amd64_gdt_t) - 1;
    gdtr->address = (u64)gdt;
}

static
void install_gdtr(amd64_gdtr_t* gdtr) {
    __asm__ volatile(
        "lgdt (%0)"
        :
        : "r"(gdtr)
        : "memory"
    );
}

static
void load_tss(u16 selector) {
    __asm__ volatile(
        "ltr %w0"
        :
        : "r"(selector)
        : "memory");
}

static
void reload_kernel_segments(u16 data_selector, u16 code_selector) {
    __asm__ volatile (
        "mov %w0, %%ds\n"
        "mov %w0, %%es\n"
        "mov %w0, %%ss\n"
        "mov %w0, %%fs\n"
        "mov %w0, %%gs\n"
        "pushq %q1\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        : 
        : "r" (data_selector), "r" ((u64)code_selector)
        : "rax", "memory"
    );
}

void amd64_cpu_gdt_init(u64 cpu_index, u64 kernel_rsp) {
    amd64_gdt_t* gdt  = &gdts[cpu_index];
    amd64_gdtr_t* gdtr = &gdtrs[cpu_index];
    amd64_tss_t* tss  = &tsses[cpu_index];

    memzero(gdt, sizeof(amd64_gdt_t));
    memzero(gdtr, sizeof(amd64_gdtr_t));
    memzero(tss, sizeof(amd64_tss_t));

    gdt_set_entry(gdt, &gdt_zero_entry, GDT_INDEX_ZERO_ENTRY);
    gdt_set_entry(gdt, &gdt_kernel_code_entry, GDT_INDEX_KERNEL_CODE);
    gdt_set_entry(gdt, &gdt_kernel_data_entry, GDT_INDEX_KERNEL_DATA);
    gdt_set_entry(gdt, &gdt_user_code32_entry, GDT_INDEX_USER_CODE32);
    gdt_set_entry(gdt, &gdt_user_data_entry, GDT_INDEX_USER_DATA);
    gdt_set_entry(gdt, &gdt_user_code_entry, GDT_INDEX_USER_CODE);

    tss->iopb_offset = sizeof(amd64_tss_t);
    tss->rsp[0] = kernel_rsp;
    set_tss(gdt, tss);

    update_gdtr(gdtr, gdt);
    install_gdtr(gdtr);

    load_tss(AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_TSS, 0));
    reload_kernel_segments(AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_KERNEL_DATA, 0), AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_KERNEL_CODE, 0));
}