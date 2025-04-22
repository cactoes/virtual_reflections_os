#include "gdt.hpp"

static gdt   g_gdt;
static gdtr  g_gdtr;
static tss_t g_tss;

/// @remarks    assumes no segment selector is needed
void __set_gdt(void* gdtr) {
    asm volatile("lgdt (%0)" : : "r"(gdtr) : "memory");
}

void __load_tss(uint64_t entry) {
    asm volatile(
        "mov %0, %%ax\n"
        "ltr %%ax\n"
        :
        : "r"((uint16_t)entry)
        : "rax", "memory"
    );
}

void gdt_init() {
    // 0x0
    memzero(&g_gdt.entries[GDT_INDEX_NULL], sizeof(gdt_entry_t));

    // kernel code 64 0x8
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].limit = 0;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].base_low16 = 0;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].base_mid8 = 0;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].access = GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READWRITE;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].granularity = GDT_LONG_MODE;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].base_high8 = 0;

    // tss entry always last entry
    g_gdt.tss_entry.length = sizeof(tss_t);
    g_gdt.tss_entry.base_low16 = (uint16_t)(((uint64_t)&g_tss) & 0xffff);
    g_gdt.tss_entry.base_mid8 = (uint8_t)(((uint64_t)&g_tss >> 16) & 0xff);
    g_gdt.tss_entry.flags1 = 0b10001001;
    g_gdt.tss_entry.flags2 = 0;
    g_gdt.tss_entry.base_high8 = (uint8_t)(((uint64_t)&g_tss >> 24) & 0xff);
    g_gdt.tss_entry.base_upper32 = (uint32_t) (((uint64_t)&g_tss >> 32) & 0xffffffff);
    g_gdt.tss_entry.reserved = 0;

    g_gdtr.limit = sizeof(g_gdt) - 1;
    g_gdtr.address = (uint64_t)&g_gdt;

    __set_gdt(&g_gdtr);
}

void tss_init() {
    memzero(&g_tss, sizeof(tss_t));

    __load_tss(GDT_INDEX_TO_ENTRY(GDT_INDEX_TSS));
}

void tss_set_rsp0(void* rsp) {
    g_tss.rsp0 = (uint64_t)rsp;
}