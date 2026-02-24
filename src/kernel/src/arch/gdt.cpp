#include "arch/generic.hpp"
#include "arch/gdt.hpp"

#ifdef ARCH_X86_64

static x86_64_gdt_t<GDT_ENTRY_COUNT>    g_gdt;
static x86_64_gdtr_t                    g_gdtr;
static x86_64_tss_t                     g_tss;

void gdt_init() {
    x86_64_gdt_init(&g_gdt);
    x86_64_gdt_set_entry(&g_gdt, &g_x86_64_zero_entry, 0);
    x86_64_gdt_set_entry(&g_gdt, &g_x86_64_kernel_64_code_entry, KERNEL_CODE_SELECTOR_INDEX);
    x86_64_gdt_set_entry(&g_gdt, &g_x86_64_kernel_64_data_entry, KERNEL_DATA_SELECTOR_INDEX);
    x86_64_gdt_set_entry(&g_gdt, &g_x86_64_user_64_data_entry, USER_DATA_SELECTOR_INDEX);
    x86_64_gdt_set_entry(&g_gdt, &g_x86_64_user_64_code_entry, USER_CODE_SELECTOR_INDEX);
    x86_64_gdt_set_tss(&g_gdt, &g_tss);
    x86_64_gdtr_update(&g_gdtr, &g_gdt);

    memzero(&g_tss, sizeof(x86_64_tss_t));
    g_tss.iomap_offset = sizeof(x86_64_tss_t);

    set_gdt(&g_gdtr);
    load_tss(X86_64_GDT_INDEX_TO_ENTRY(X86_64_GDT_INDEX_TSS(GDT_ENTRY_COUNT)));
}

void gdt_set_stack_pointer0(void* p_stack_pointer) {
    x86_64_tss_set_stack_pointer0(&g_tss, p_stack_pointer);
}

uint64_t gdt_get_kernel_code_selector() {
    return X86_64_GDT_INDEX_TO_ENTRY(KERNEL_CODE_SELECTOR_INDEX);
}

#endif // ARCH_X86_64