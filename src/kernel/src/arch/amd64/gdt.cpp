#include "arch/amd64/gdt.hpp"

void amd64_gdt_init(amd64_gdt_t* gdt) {
    memzero(gdt, sizeof(amd64_gdt_t));
}

void amd64_gdt_set_entry(amd64_gdt_t* gdt, amd64_gdt_entry_t* entry, u64 index) {
    if (index >= GDT_ENTRY_COUNT)
        return;

    memcpy(&gdt->entries[index], entry, sizeof(amd64_gdt_entry_t));
}

void amd64_gdt_set_tss(amd64_gdt_t* gdt, amd64_tss_t* tss) {
    gdt->tss_entry.length = sizeof(amd64_tss_t) - 1;
    gdt->tss_entry.base_low16 = (u16)(((u64)tss) & MAX_UINT16);
    gdt->tss_entry.base_mid8 = (u8)(((u64)tss >> 16) & MAX_UINT8);
    gdt->tss_entry.flags1 = 0b10001001;
    gdt->tss_entry.flags2 = 0;
    gdt->tss_entry.base_high8 = (u8)(((u64)tss >> 24) & MAX_UINT8);
    gdt->tss_entry.base_upper32 = (u32)(((u64)tss >> 32) & MAX_UINT32);
    gdt->tss_entry.reserved = 0;
}

void amd64_tss_set_stack_pointer0(amd64_tss_t* tss, void* stack_pointer) {
    tss->rsp0 = (u64)stack_pointer;
}

void amd64_gdtr_update(amd64_gdtr_t* gdtr, amd64_gdt_t* gdt) {
    gdtr->limit = sizeof(amd64_gdt_t) - 1;
    gdtr->address = (u64)gdt;
}

u16 amd64_get_selector_for(u16 index) {
    if (index >= GDT_ENTRY_COUNT)
        return 0;

    return AMD64_GDT_INDEX_TO_ENTRY(index);
}