//==========================================
/// @file       amd64_entry.cpp
/// @brief      amd64 c entry point that sets up the actual kernel etc
///             duplicate defines are possible sinze we try to avoid imports
//==========================================

#include "arch/amd64/msr.hpp"
#include "arch/amd64/gdt.hpp"
#include "common2.hpp"

/// @brief  generic defines
#define PAGE_SIZE_LARGE             0x200000

#define PF_PRESENT                  (1 << 0)
#define PF_READ_WRITE               (1 << 1)
#define PF_PAGE_SIZE                (1 << 7)

#define KERNEL_VIRTUAL_BASE_ADDRESS 0xFFFFF80000000000

/// @brief  generic types
//

extern "C" void virtual_kernel_entry(struct multiboot_t*, void*);
extern "C" void amd64_syscall_stub();

/// @brief  linker variables get placed here and the linter is disabled for them
// NOLINTBEGIN
namespace linker_variables {

extern "C" u8 __lnk_bss_start;
extern "C" u8 __lnk_bss_size;

extern "C" u8 __lnk_init_array_start;
extern "C" u8 __lnk_init_array_end;

extern "C" u8 __lnk_kernel_end_physical;

extern "C" u8 page_table_l4;

} // namespace linker_variables
// NOLINTEND

/// @brief  page table level 3 for jumping to high memory, pdp
__attribute__((section(".boot.bss"), aligned(4096)))
u8 page_table_l3_jmp[0x1000];

/// @brief  page table level 2 for jumping to high memory, pd
__attribute__((section(".boot.bss"), aligned(4096)))
u8 page_table_l2_jmp[0x1000];

__attribute__((section(".bss")))
amd64_gdt_t gdt;

__attribute__((section(".bss")))
amd64_gdtr_t gdtr;

__attribute__((section(".bss")))
amd64_tss_t tss;

/// @brief              local memzero define defined in boot text section
/// @param[in] address  pointer to address to clear
/// @param size         size of region to clear
static
__attribute__((section(".boot.text")))
void amd64_memzero(void* address, u64 size) {
    asm volatile (
        "xor %%al, %%al\n"
        "rep stosb\n"
        : "+D"(address), "+c"(size)
        :
        : "al", "memory"
    );
}

/// @brief  call all global constructors
static
__attribute__((section(".boot.text")))
void amd64_call_constructors() {
    void(**init_array_start)() = (void(**)())&linker_variables::__lnk_init_array_start;
    void(**init_array_end)() = (void(**)())&linker_variables::__lnk_init_array_end;

    for (void (**constructor)() = init_array_start; constructor < init_array_end; constructor++)
        if (constructor)    
            (*constructor)();
}

static
__attribute__((section(".boot.text")))
void amd64_reload_page_table() {
    asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
}

// TODO @since 19/05/2026 -- 01:41
// move
extern struct cpu_t* get_current_cpu();

// TODO @since 19/05/2026 -- 01:57
// move
amd64_tss_t* amd64_get_tss() {
    return &tss;
}

static
__attribute__((section(".text")))
void amd64_init_msr() {
    u64 star = ((u64)(USER_CODE_32_SELECTOR_INDEX << 3) << 48)
              | ((u64)(KERNEL_CODE_SELECTOR_INDEX << 3) << 32);

    amd64_msr_enable_sce();
    amd64_msr_set_star(star);
    amd64_msr_set_lstar((void*)amd64_syscall_stub);
    amd64_msr_set_sf_mask(RFLAGS_TF | RFLAGS_IF | RFLAGS_DF);
    amd64_msr_set_gs_base(get_current_cpu());
    amd64_msr_set_kernel_gs_base(nullptr);
}

static
__attribute__((section(".text")))
void amd64_init_gdt() {
    amd64_gdt_init(&gdt);
    amd64_gdt_set_entry(&gdt, &g_amd64_zero_entry, GDT_ZERO_ENTRY);
    amd64_gdt_set_entry(&gdt, &g_amd64_kernel_64_code_entry, KERNEL_CODE_SELECTOR_INDEX);
    amd64_gdt_set_entry(&gdt, &g_amd64_kernel_64_data_entry, KERNEL_DATA_SELECTOR_INDEX);
    amd64_gdt_set_entry(&gdt, &g_amd64_user_32_code_entry, USER_CODE_32_SELECTOR_INDEX);
    amd64_gdt_set_entry(&gdt, &g_amd64_user_64_data_entry, USER_DATA_SELECTOR_INDEX);
    amd64_gdt_set_entry(&gdt, &g_amd64_user_64_code_entry, USER_CODE_SELECTOR_INDEX);
    amd64_gdt_set_tss(&gdt, &tss);
    amd64_gdtr_update(&gdtr, &gdt);

    memzero(&tss, sizeof(amd64_tss_t));
    tss.iomap_offset = sizeof(amd64_tss_t);

    amd64_set_gdt(&gdtr);
    amd64_load_tss(AMD64_GDT_INDEX_TO_ENTRY(AMD64_GDT_INDEX_TSS(GDT_ENTRY_COUNT)));
}

/// @brief                          amd64 boot entry, the assembly jumps to here to continue the setup
/// @param[in] multiboot2_struct    pointer to the multiboot 2 structure
extern "C"
__attribute__((section(".boot.text")))
[[noreturn]]
void amd64_entry(void* multiboot2_struct) {
    // asm volatile (
    //     "mov   $0,    %%ax\n"
    //     "mov   %%ax,  %%ss\n"
    //     "mov   %%ax,  %%ds\n"
    //     "mov   %%ax,  %%es\n"
    //     "mov   %%ax,  %%fs\n"
    //     "mov   %%ax,  %%gs\n"
    //     ::: "ax", "memory"
    // );

    // recursively map the page table
    ((u64*)&linker_variables::page_table_l4)[511] = ((u64)&linker_variables::page_table_l4 & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;

    // calculate kernel page count
    u64 page_count = ((u64)&linker_variables::__lnk_kernel_end_physical + (PAGE_SIZE_LARGE - 1)) >> 21;

    // also map the kernel in high memory
    ((u64*)&linker_variables::page_table_l4)[496] = ((u64)page_table_l3_jmp & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;
    ((u64*)page_table_l3_jmp)[0] = ((u64)page_table_l2_jmp & ~0xFFF) | PF_PRESENT | PF_READ_WRITE;
    for (u64 i = 0; i < page_count; i++)
        ((u64*)page_table_l2_jmp)[i] = (i << 21) | PF_PRESENT | PF_READ_WRITE | PF_PAGE_SIZE;

    // force update the page table
    amd64_reload_page_table();

    //
    // ================== high memory is now mapped ==================
    //

    // zero bss
    amd64_memzero((void*)&linker_variables::__lnk_bss_start, (u64)&linker_variables::__lnk_bss_size);

    // jump to new stack
    u64 new_stack;
    asm volatile (
        "mov %%rsp, %0\n\t"
        "add %1, %0\n\t"
        "mov %0, %%rsp"
        : "=&r"(new_stack)
        : "r"(KERNEL_VIRTUAL_BASE_ADDRESS)
        : "memory"
    );

    // 4. initialze cpu0 (idt, gdt, msr)

    // gdt
    amd64_init_gdt();

    // idt

    // msr
    amd64_init_msr();

    // call all c++ global constructors
    amd64_call_constructors();

    // jump to virtual kernel entrypoint
    virtual_kernel_entry((struct multiboot_t*)((u64)multiboot2_struct + KERNEL_VIRTUAL_BASE_ADDRESS), (void*)&linker_variables::page_table_l4);
    while (true);
}