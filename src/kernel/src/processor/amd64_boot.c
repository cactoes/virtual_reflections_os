#include "libtype.h"
#include "libdef.h"
#include "processor/amd64_linker.h"

// in main.c
void virtual_kernel_entry(void* multiboot2);

static
__section_text
void __memzero(void* dst, u64 size) {
    __asm__ volatile ("rep stosb" : "+D"(dst), "+c"(size) : "a"(0) : "memory");
}

static
__section_text
void __reload_page_table() {
    u64 cr3;
    __asm__ volatile (
        "mov %%cr3, %0\n\t"
        "mov %0, %%cr3"
        : "=&r"(cr3)
        :
        : "memory"
    );
}

__noreturn __section_text
void amd64_entry(void* multiboot2) {
    __memzero((void*)LNK_BSS_START, LNK_BSS_SIZE);

    ((u64*)KERNEL_PHYS_TO_VIRT(page_table_l4))[0] = 0;
    __reload_page_table();

    virtual_kernel_entry(multiboot2);
    while (true);
}