#include "crash_handler.hpp"

#include "drivers/vga.hpp"
#include "std/string.hpp"
#include "utils/debug.hpp"
#include "virtual_thread.hpp"
#include "io.hpp"
#include "utils/mutex.hpp"
#include "interrupt_manager.hpp"
#include "arch/amd64/cpu.hpp"
#include "drivers/graphics/graphics_driver.hpp"

volatile u64 global_online_systems_flags = 0;

NORETURN void kernel_fatal_end() {
    while (true)
        debug_trap("kernel fatal");
}

void kernel_fatal_internal(u64 code, const char* message, interrupt_regs_t* cpu_state) {
    disable_interrupts();

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
                u16 seg_index = cpu_state->error_code >> 3;
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

    auto temp_handle = __thread_tls ? __thread_tls->handle : VTHREAD_HANDLE_INVALID;

    // if not main thread just terminate the thread not the system
    // & if a valid tls is setup
    if (__thread_tls && __thread_tls->handle != VTHREAD_MAIN_THREAD_HANDLE && __thread_tls->handle != VTHREAD_HANDLE_INVALID) {
        kprintf("[thread %ul]: terminated (crashed or forcefully stopped). code: 0x%uh (%s).\n", __thread_tls->handle, code, message);
        mutex_clear_all_thread_references_and_release(__thread_tls->handle);

        // to prevent crash looping
        if (vthread_get_state() != vthread_state_t::STOPPING)
            vthread_terminate();
    }

    graphics_driver_t* gd = get_global_graphics_driver();

    size_t w, h;
    graphics_driver_get_size(gd, &w, &h);
    graphics_driver_draw_square(gd, 0, 0, w, h, { 0, 0, 0 });

    size_t w2, h2;
    graphics_driver_get_text_size(gd, " Corrupted Beyond Repair ", &w2, &h2);
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100, " Corrupted Beyond Repair ", { 0, 0, 0 }, { 255, 255, 255 });
    
    // we take this since its the longes piece of text
    graphics_driver_get_text_size(gd, "Additional diagnostic information generated (if available).", &w2, &h2);

    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 2, "The system encounterd a critical error.", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 3, "Immediate recovery is not possible.", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 5, "Additional diagnostic information generated (if available).", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 6, "A system reboot is required.", { 255, 255, 255 });

    char buffer[256] {};
    sprintf(buffer, 256, "Stop code: 0x%uh. Systems initialized: %ul", code, global_online_systems_flags);
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 8, buffer, { 255, 255, 255 });

    if (code == 0xe) {
        bool is_reason_protection = (cpu_state->error_code & 0x1);
        bool is_operation_write = (cpu_state->error_code & 0x2);
        bool is_mode_user = (cpu_state->error_code & 0x4);

        char buffer[256] {};
        sprintf(buffer, 256, "address: 0x%p", amd64_read_cr2());
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 9, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "reason: %s", is_reason_protection ? "protection violation" : "non-present page");
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 10, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "operation: %s", is_operation_write ? "write" : "read");
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 11, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "mode: %s", is_mode_user ? "user" : "kernel");
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 12, buffer, { 255, 255, 255 });
    } else if (code == 0xd) {
        char buffer[256] {};
        sprintf(buffer, 256, "rsp: 0x%p [% 16 == %ul]", cpu_state->rsp, (u64)(cpu_state->rsp % 16));
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 9, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "rbp: 0x%p [% 16 == %ul]", cpu_state->rbp, (u64)(cpu_state->rbp % 16));
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 10, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "rip: 0x%p [% 16 == %ul]", cpu_state->rip, (u64)(cpu_state->rip % 16));
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 11, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "error code: %ul", cpu_state->error_code);
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 12, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "cr3: %ul", amd64_read_cr3());
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 13, buffer, { 255, 255, 255 });

        sprintf(buffer, 256, "handle: %ul", temp_handle);
        graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 14, buffer, { 255, 255, 255 });
    }

    graphics_driver_render(gd);

    // reboot?

    kernel_fatal_end();
}
