//==========================================
/// @file       amd64_entry.cpp
/// @brief      amd64 c entry point that sets up the actual kernel etc
///             duplicate defines are possible sinze we try to avoid imports
//==========================================

#include "arch/amd64/msr.hpp"
#include "arch/amd64/gdt.hpp"
#include "arch/amd64/idt.hpp"
#include "arch/amd64/cpu.hpp"
#include "arch/amd64/port.hpp"
#include "arch/amd64/apic.hpp"
#include "arch/arch_selector.hpp"
#include "memory/vmem.hpp"
#include "memory/paging.hpp"
#include "interrupt_manager.hpp"
#include "syscall_handler.hpp"
#include "common.hpp"
#include "crash_handler.hpp"
#include "io.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

/// @brief  generic defines
#define PF_PRESENT                  (1 << 0)
#define PF_READ_WRITE               (1 << 1)
#define PF_PAGE_SIZE                (1 << 7)

#define KERNEL_VIRTUAL_BASE_ADDRESS 0xFFFFF80000000000

/// @brief  generic types

/// @brief  extern functions
extern "C" void virtual_kernel_entry(struct multiboot_t*);
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

__attribute__((section(".bss")))
amd64_idt_entry_t idt[AMD64_INT_IDT_ENTRY_COUNT];

__attribute__((section(".bss")))
amd64_idt_register_t idtr;

/// @brief      returns a pointer to the tss
/// @return     pinter to tss struct
__attribute__((section(".text")))
amd64_tss_t* amd64_get_tss() {
    return &tss;
}

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
    amd64_reload_segments(AMD64_GDT_INDEX_TO_ENTRY(KERNEL_CODE_SELECTOR_INDEX), AMD64_GDT_INDEX_TO_ENTRY(KERNEL_DATA_SELECTOR_INDEX));
}

extern "C"
__attribute__((section(".text")))
interrupt_t amd64_convert_to_interrupt(u64 code) {
    switch (code) {
        case 0: return interrupt_t::EXCEPTION_DIVISION_BY_ZERO;
        case 1: return interrupt_t::EXCEPTION_SINGLE_STEP_INTERRUPT;
        case 2: return interrupt_t::EXCEPTION_NMI;
        case 3: return interrupt_t::EXCEPTION_BREAKPOINT;
        case 4: return interrupt_t::EXCEPTION_OVERFLOW;
        case 5: return interrupt_t::EXCEPTION_BOUND_RANGE_EXCEEDED;
        case 6: return interrupt_t::EXCEPTION_INVALID_OPCODE;
        case 7: return interrupt_t::EXCEPTION_COPROCESSOR_NOT_AVAILABLE;
        case 8: return interrupt_t::EXCEPTION_DOUBLE_FAULT;
        case 9: return interrupt_t::EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN;
        case 10: return interrupt_t::EXCEPTION_INVALID_TSS;
        case 11: return interrupt_t::EXCEPTION_SEGMENT_NOT_PRESENT;
        case 12: return interrupt_t::EXCEPTION_STACK_SEGMENT_FAULT;
        case 13: return interrupt_t::EXCEPTION_GENERAL_PROTECTION_FAULT;
        case 14: return interrupt_t::EXCEPTION_PAGE_FAULT;
        case 15: return interrupt_t::EXCEPTION_RESERVED;
        case 16: return interrupt_t::EXCEPTION_X87_FLOATING_POINT_EXCEPTION;
        case 17: return interrupt_t::EXCEPTION_ALIGNMENT_CHECK;
        case 18: return interrupt_t::EXCEPTION_MACHINE_CHECK;
        case 19: return interrupt_t::EXCEPTION_SIMD_FP_EXCEPTION;
        case 20: return interrupt_t::EXCEPTION_VIRTUALIZATION_EXCEPTION;
        case 21: return interrupt_t::EXCEPTION_CONTROL_PROTECTION_EXCEPTION;

        case 32: return interrupt_t::HARDWARE_PIT;
        case 33: return interrupt_t::HARDWARE_KEYBOARD;
        case 34: return interrupt_t::HARDWARE_CASCADE;
        case 35: return interrupt_t::HARDWARE_COM2;
        case 36: return interrupt_t::HARDWARE_COM1;
        case 37: return interrupt_t::HARDWARE_LPT2;
        case 38: return interrupt_t::HARDWARE_FLOPPY_DISK;
        case 39: return interrupt_t::HARDWARE_LPT1;
        case 40: return interrupt_t::HARDWARE_CMOS_RTC;
        case 41: return interrupt_t::HARDWARE_FFP_L_SCSI_NIC;
        case 42: return interrupt_t::HARDWARE_FFP_SSCI_NIC1;
        case 43: return interrupt_t::HARDWARE_FFP_SSCI_NIC2;
        case 44: return interrupt_t::HARDWARE_PS2_MOUSE;
        case 45: return interrupt_t::HARDWARE_COPROCESSOR;
        case 46: return interrupt_t::HARDWARE_PRIMARY_ATA_HD;
        case 47: return interrupt_t::HARDWARE_SECONDARY_ATA_HD;

        case 129: return interrupt_t::SOFTWARE_SCHEDULER;

        default: return interrupt_t::UNKOWN;
    }

    return interrupt_t::UNKOWN;
}

// TODO @since 04/06/2026 -- 08:19
// replace with actual getter
extern void* global_lapic;

extern "C"
__attribute__((section(".text")))
interrupt_regs_t* amd64_interrupt_dispatch(u64 code, interrupt_regs_t* stack) {
    extern volatile bool global_is_in_interupt;
    global_is_in_interupt = true;

    interrupt_t interrupt = amd64_convert_to_interrupt(code);

    if (interrupt == interrupt_t::UNKOWN) {
        global_is_in_interupt = false;
        return stack;
    }

    interrupt_regs_t* result_stack = (interrupt_regs_t*)interrupt_manager_dispatch(interrupt, (void*)stack);

    if (code >= 32 && code <= 47) {
        if (global_lapic)
            *((volatile u32*)((u8*)global_lapic + AMD64_APIC_EOI)) = 0;
        else
            amd64_interrupt_send_eoi(code - 0x20);
    }

    global_is_in_interupt = false;
    return result_stack;
}

static
__attribute__((section(".text")))
void amd64_init_idt() {
    amd64_set_idtr(&idtr, idt);
    amd64_set_idt_entries(idt, amd64_get_selector_for(KERNEL_CODE_SELECTOR_INDEX));

    amd64_out_port8(AMD64_INT_PIC1, 0x11);
    amd64_out_port8(AMD64_INT_PIC2, 0x11);

    amd64_out_port8(AMD64_INT_PIC1_DATA, 0x20);
    amd64_out_port8(AMD64_INT_PIC2_DATA, 0x28);

    amd64_out_port8(AMD64_INT_PIC1_DATA, 4);
    amd64_out_port8(AMD64_INT_PIC2_DATA, 2);

    amd64_out_port8(AMD64_INT_PIC1_DATA, 1);
    amd64_out_port8(AMD64_INT_PIC2_DATA, 1);

    static const u8 irqs_to_unmask[] = {
        AMD64_INT_IRQ_PIT,
        AMD64_INT_IRQ_PS2_KEYBOARD,
        AMD64_INT_IRQ_PS2_MOUSE
    };

    for (size_t i = 0; i < sizeof(irqs_to_unmask); i++)
        amd64_irq_unmask(irqs_to_unmask[i]);

    amd64_flush_idt(idtr);
    amd64_interrupts_enable();
}

extern "C"
__attribute__((section(".text")))
u64 amd64_syscall_dispatch(u64 syscall_num, syscall_regs_t* regs) {
    return syscall_dispatch(syscall_num,
        (void*)regs->rdi,
        (void*)regs->rsi,
        (void*)regs->rdx,
        (void*)regs->r10,
        (void*)regs->r8,
        (void*)regs->r9);
}

extern "C"
__attribute__((section(".text")))
void amd64_crash_handler(u64 crash_code, const char* message, interrupt_regs_t* stack) {
    kprintf("exception: ");
    switch (crash_code) {
        case 0x0: kprintf("DIVISION_BY_ZERO"); break;
        case 0x1: kprintf("SINGLE_STEP_INTERRUPT"); break;
        case 0x2: kprintf("NMI"); break;
        case 0x3: kprintf("BREAKPOINT"); break;
        case 0x4: kprintf("OVERFLOW"); break;
        case 0x5: kprintf("BOUND_RANGE_EXCEEDED"); break;
        case 0x6: kprintf("INVALID_OPCODE"); break;
        case 0x7: kprintf("COPROCESSOR_NOT_AVAILABLE"); break;
        case 0x8: kprintf("DOUBLE_FAULT"); break;
        case 0x9: kprintf("COPROCESSOR_SEGMENT_OVERRUN"); break;
        case 0xA: kprintf("INVALID_TSS"); break;
        case 0xB: kprintf("SEGMENT_NOT_PRESENT"); break;
        case 0xC: kprintf("STACK_SEGMENT_FAULT"); break;
        case 0xD: {
            if (stack) {
                u16 seg_index = stack->error_code >> 3;
                bool is_external = stack->error_code & 0x1;
                bool is_idt = stack->error_code & 0x2;
    
                kprintf("GENERAL_PROTECTION_FAULT\n        extenal:%u\n        table:%s\n        segment index: 0x%uh", is_external, is_idt ? "idt/ldt" : "gdt", seg_index);
            } else {
                kprintf("GENERAL_PROTECTION_FAULT");
            }
            break;
        }
        case 0xE:{
            if (stack) {
                bool is_reason_protection = (stack->error_code & 0x1);
                bool is_operation_write = (stack->error_code & 0x2);
                bool is_mode_user = (stack->error_code & 0x4);
    
                kprintf("PAGE_FAULT\n        address: 0x%p\n        reason:%s\n        operation:%s\n        mode:%s",
                    amd64_read_cr2(),
                    is_reason_protection ? "protection violation" : "non-present page",
                    is_operation_write ? "write" : "read",
                    is_mode_user ? "user" : "kernel");
            } else {
                kprintf("PAGE_FAULT");
            }

            break;
        }
        case 0xF: kprintf("RESERVED"); break;
        case 0x10: kprintf("X87_FLOATING_POINT_EXCEPTION"); break;
        case 0x11: kprintf("ALIGNMENT_CHECK"); break;
        case 0x12: kprintf("MACHINE_CHECK"); break;
        case 0x13: kprintf("SIMD_FP_EXCEPTION"); break;
        case 0x14: kprintf("VIRTUALIZATION_EXCEPTION"); break;
        case 0x15: kprintf("CONTROL_PROTECTION_EXCEPTION"); break;
        case KERNEL_FATAL_KERNEL_EXITED: kprintf("KERNEL_FATAL_KERNEL_EXITED"); break;
        case KERNEL_FATAL_CRITICAL_THREAD_DIED: kprintf("KERNEL_FATAL_CRITICAL_THREAD_DIED"); break;
        case KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE: kprintf("KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE"); break;
        case KERNEL_FATAL_VMEM_INIT: kprintf("KERNEL_FATAL_VMEM_INIT"); break;
        case KERNEL_FATAL_HEAP_INIT: kprintf("KERNEL_FATAL_HEAP_INIT"); break;
        case KERNEL_FATAL_VTHREAD_INIT: kprintf("KERNEL_FATAL_VTHREAD_INIT"); break;
        case KERNEL_FATAL_VTHREAD_STACK_PROTECTION: kprintf("KERNEL_FATAL_VTHREAD_STACK_PROTECTION"); break;
        default: kprintf("UNKOWN"); break;
    }

    kprintf("\n");

    kprintf("cpu dump:\n");
    kprintf("    cr2:        0x%uh\n", amd64_read_cr2());
    if (stack) {
        kprintf("    rflags:     0x%uh\n", stack->rflags);
        kprintf("    error code: 0x%uh\n", stack->error_code);
    
        kprintf("    registers:\n");
        kprintf("        r8:  0x%uh\n", stack->r8);
        kprintf("        r9:  0x%uh\n", stack->r9);
        kprintf("        r10: 0x%uh\n", stack->r10);
        kprintf("        r11: 0x%uh\n", stack->r11);
        kprintf("        r12: 0x%uh\n", stack->r12);
        kprintf("        r13: 0x%uh\n", stack->r13);
        kprintf("        r14: 0x%uh\n", stack->r14);
        kprintf("        r15: 0x%uh\n", stack->r15);
        kprintf("        rax: 0x%uh\n", stack->rax);
        kprintf("        rcx: 0x%uh\n", stack->rcx);
        kprintf("        rdx: 0x%uh\n", stack->rdx);
        kprintf("        rbp: 0x%uh\n", stack->rbp);
        kprintf("        rsi: 0x%uh\n", stack->rsi);
        kprintf("        rdi: 0x%uh\n", stack->rdi);
        kprintf("        rip: 0x%uh\n", stack->rip);
        kprintf("        rsp: 0x%uh\n", stack->rsp);
    }
}

[[noreturn]]
static
__attribute__((section(".text")))
void amd64_kernel_jump_stub(void* mbs) {
    // unmap the boot code
    const u64 kernel_page_count = ((u64)&linker_variables::__lnk_kernel_end_physical + (PAGE_SIZE_LARGE - 1)) >> 21;
    for (u64 i = 0; i < kernel_page_count; i++)
        vmem_unmap_2mb((void*)(i * PAGE_SIZE_LARGE));

    // jump to virtual kernel entrypoint
    virtual_kernel_entry((struct multiboot_t*)mbs);

    // just in case
    while (true);
}

/// @brief                          amd64 boot entry, the assembly jumps to here to continue the setup
/// @param[in] multiboot2_struct    pointer to the multiboot 2 structure
extern "C"
[[noreturn]]
__attribute__((section(".boot.text")))
void amd64_entry(void* multiboot2_struct) {
    asm volatile (
        "mov   $0,    %%ax\n"
        "mov   %%ax,  %%ss\n"
        "mov   %%ax,  %%ds\n"
        "mov   %%ax,  %%es\n"
        "mov   %%ax,  %%fs\n"
        "mov   %%ax,  %%gs\n"
        ::: "ax", "memory"
    );

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

    amd64_init_gdt();
    amd64_init_idt();
    amd64_init_msr();

    // call all c++ global constructors
    amd64_call_constructors();

    // init virtual memory
    pmem_init(multiboot2_struct);

    // almost ready to jump to the kernel code
    amd64_kernel_jump_stub((void*)((u64)multiboot2_struct + KERNEL_VIRTUAL_BASE_ADDRESS));
}

#endif