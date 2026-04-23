#include "crash_handler.hpp"
#include "arch/generic.hpp"
#include "arch/interrupt.hpp"
#include "drivers/vga.hpp"
#include "std/string.hpp"
#include "utils/debug.hpp"
#include "virtual_thread.hpp"
#include "io.hpp"
#include "utils/mutex.hpp"

NORETURN void kernel_fatal_end() {
    while (true)
        debug_trap("kernel fatal");
}

void kernel_fatal_internal(uint64_t code, const char* message, interrupt_regs_t* cpu_state) {
    cli();

    kprintf("kernel fatal triggerd!\n");
    kprintf("code: 0x%uh (%s).\n", code, message);
    kprintf("    exception: ");
    switch (code) {
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
            if (cpu_state) {
                uint16_t seg_index = cpu_state->error_code >> 3;
                bool is_external = cpu_state->error_code & 0x1;
                bool is_idt = cpu_state->error_code & 0x2;
    
                kprintf("GENERAL_PROTECTION_FAULT\n        extenal:%u\n        table:%s\n        segment index: 0x%uh", is_external, is_idt ? "idt/ldt" : "gdt", seg_index);
            } else {
                kprintf("GENERAL_PROTECTION_FAULT");
            }
            break;
        }
        case 0xE:{
            if (cpu_state) {
                bool is_reason_protection = (cpu_state->error_code & 0x1);
                bool is_operation_write = (cpu_state->error_code & 0x2);
                bool is_mode_user = (cpu_state->error_code & 0x4);
    
                kprintf("PAGE_FAULT\n        address: 0x%p\n        reason:%s\n        operation:%s\n        mode:%s",
                    read_cr2(),
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
    kprintf("    cr2:        0x%uh\n", read_cr2());
    if (cpu_state) {
        kprintf("    rflags:     0x%uh\n", cpu_state->rflags);
        kprintf("    error code: 0x%uh\n", cpu_state->error_code);
    
        kprintf("    registers:\n");
        kprintf("        r8:  0x%uh\n", cpu_state->r8);
        kprintf("        r9:  0x%uh\n", cpu_state->r9);
        kprintf("        r10: 0x%uh\n", cpu_state->r10);
        kprintf("        r11: 0x%uh\n", cpu_state->r11);
        kprintf("        r12: 0x%uh\n", cpu_state->r12);
        kprintf("        r13: 0x%uh\n", cpu_state->r13);
        kprintf("        r14: 0x%uh\n", cpu_state->r14);
        kprintf("        r15: 0x%uh\n", cpu_state->r15);
        kprintf("        rax: 0x%uh\n", cpu_state->rax);
        kprintf("        rcx: 0x%uh\n", cpu_state->rcx);
        kprintf("        rdx: 0x%uh\n", cpu_state->rdx);
        kprintf("        rbp: 0x%uh\n", cpu_state->rbp);
        kprintf("        rsi: 0x%uh\n", cpu_state->rsi);
        kprintf("        rdi: 0x%uh\n", cpu_state->rdi);
        kprintf("        rip: 0x%uh\n", cpu_state->rip);
        kprintf("        rsp: 0x%uh\n", cpu_state->rsp);
    }

    // if not main thread just terminate the thread not the system
    // & if a valid tls is setup
    if (__thread_tls && __thread_tls->handle != VTHREAD_MAIN_THREAD_HANDLE && __thread_tls->handle != VTHREAD_HANDLE_INVALID) {
        kprintf("[thread %ul]: terminated (crashed or forcefully stopped). code: 0x%uh (%s).\n", __thread_tls->handle, code, message);
        mutex_clear_all_thread_references_and_release(__thread_tls->handle);

        // to prevent crash looping
        if (vthread_get_state() != vthread_state_t::STOPPING)
            vthread_terminate();
    }

    // assume we are still in vga text mode
    vga_tm_color_map_t color_default {};
    color_default.foreground = vga_tm_color_t::WHITE;
    color_default.background = vga_tm_color_t::BLACK;

    vga_tm_color_map_t color_highlight {};
    color_highlight.foreground = vga_tm_color_t::BLACK;
    color_highlight.background = vga_tm_color_t::WHITE;

    vga_tm_set_color(&g_vga_tm_buffer, &color_default);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    vga_tm_set_cursor(&g_vga_tm_buffer, 27, 2);
    vga_tm_puts_color(&g_vga_tm_buffer, &color_highlight, " Corrupted Beyond Repair ");

    vga_tm_set_cursor(&g_vga_tm_buffer, 0, 5);
    vga_tm_puts(&g_vga_tm_buffer, "        The system encounterd a critical error.\n");
    vga_tm_puts(&g_vga_tm_buffer, "        Immediate recovery is not possible.\n\n");
    vga_tm_puts(&g_vga_tm_buffer, "        Additional diagnostic information generated (if available).\n");
    vga_tm_puts(&g_vga_tm_buffer, "        A system reboot is required.\n\n");
    
    char buffer[256] {};
    sprintf(buffer, 256, "        Stop code: 0x%uh. ", code);
    vga_tm_puts(&g_vga_tm_buffer, buffer);

    // reboot?

    kernel_fatal_end();
}
