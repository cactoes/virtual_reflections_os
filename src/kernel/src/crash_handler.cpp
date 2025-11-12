#include "crash_handler.hpp"
#include "arch/generic.hpp"
#include "drivers/vga.hpp"
#include "std/string.hpp"
#include "utils/debug.hpp"
#include "virtual_thread.hpp"
#include "io.hpp"

void __kernel_fatal(uint64_t code, const char* p_message, cpu_state_t* p_cpu_state) {
    io_flag_set(io_flag::KPRINT_BYPASS_VFS, true);

    kprintf("kernel fatal triggerd: 0x%uh \"%s\"\n", code, p_message);

    // if not main thread just terminate the thread not the system
    // & if a valid tls is setup
    if (auto tls = vthread_get_tls()) {
        if (auto handle = tls->handle; tls->handle != VTHREAD_MAIN_THREAD_HANDLE) {
            kprintf("thread: %ul terminated (crashed or forcefully stopped)\n", tls->handle);
            mutex_clear_all_thread_references_and_release(handle);
            vthread_terminate();
        }
    }

    vga_tm_color_map_t color {};
    color.foreground = vga_tm_color_t::WHITE;
    color.background = vga_tm_color_t::BLACK;

    vga_tm_set_color(&g_vga_tm_buffer, &color);
    vga_tm_clear_buffer(&g_vga_tm_buffer);
    vga_tm_puts(&g_vga_tm_buffer, "*** KERNEL FATAL ***\n");

    char buffer[256];
    sprintf(buffer, ARRAY_LENGTH(buffer), "> ERROR CODE: 0x%uh\n", code);
    vga_tm_puts(&g_vga_tm_buffer, buffer);

    if (code >= 0 && code <= 0x15) {
        vga_tm_puts(&g_vga_tm_buffer, "> ");
        switch (code) {
            case 0x0: vga_tm_puts(&g_vga_tm_buffer, "DIVISION_BY_ZERO\n"); break;
            case 0x1: vga_tm_puts(&g_vga_tm_buffer, "SINGLE_STEP_INTERRUPT\n"); break;
            case 0x2: vga_tm_puts(&g_vga_tm_buffer, "NMI\n"); break;
            case 0x3: vga_tm_puts(&g_vga_tm_buffer, "BREAKPOINT\n"); break;
            case 0x4: vga_tm_puts(&g_vga_tm_buffer, "OVERFLOW\n"); break;
            case 0x5: vga_tm_puts(&g_vga_tm_buffer, "BOUND_RANGE_EXCEEDED\n"); break;
            case 0x6: vga_tm_puts(&g_vga_tm_buffer, "INVALID_OPCODE\n"); break;
            case 0x7: vga_tm_puts(&g_vga_tm_buffer, "COPROCESSOR_NOT_AVAILABLE\n"); break;
            case 0x8: vga_tm_puts(&g_vga_tm_buffer, "DOUBLE_FAULT\n"); break;
            case 0x9: vga_tm_puts(&g_vga_tm_buffer, "COPROCESSOR_SEGMENT_OVERRUN\n"); break;
            case 0xA: vga_tm_puts(&g_vga_tm_buffer, "INVALID_TSS\n"); break;
            case 0xB: vga_tm_puts(&g_vga_tm_buffer, "SEGMENT_NOT_PRESENT\n"); break;
            case 0xC: vga_tm_puts(&g_vga_tm_buffer, "STACK_SEGMENT_FAULT\n"); break;
            case 0xD: {
                uint16_t seg_index = p_cpu_state->error_code >> 3;
                bool is_external = p_cpu_state->error_code & 0x1;
                bool is_idt = p_cpu_state->error_code & 0x2;

                memzero(buffer, ARRAY_LENGTH(buffer));
                sprintf(buffer, ARRAY_LENGTH(buffer), "GENERAL_PROTECTION_FAULT\nEXTERNAL[%u] TABLE[%s] SEGMENT INDEX: [0x%uh]", is_external, is_idt ? "IDT/LDT" : "GDT", seg_index);
                vga_tm_puts(&g_vga_tm_buffer, buffer);
                break;
            }
            case 0xE:
                memzero(buffer, ARRAY_LENGTH(buffer));
                sprintf(buffer, ARRAY_LENGTH(buffer), "PAGE_FAULT @ 0x%uh\n\nREASON    : %s\nOPERATION : %s\nMODE      : %s\n", read_cr2(),
                    (p_cpu_state->error_code & 0x1) ? "PROTECTION VIOLATION" : "NON-PRESENT PAGE",
                    (p_cpu_state->error_code & 0x2) ? "WRITE" : "READ",
                    (p_cpu_state->error_code & 0x4) ? "USER" : "KERNEL");
                vga_tm_puts(&g_vga_tm_buffer, buffer);
                break;
            case 0xF: vga_tm_puts(&g_vga_tm_buffer, "RESERVED\n"); break;
            case 0x10: vga_tm_puts(&g_vga_tm_buffer, "X87_FLOATING_POINT_EXCEPTION\n"); break;
            case 0x11: vga_tm_puts(&g_vga_tm_buffer, "ALIGNMENT_CHECK\n"); break;
            case 0x12: vga_tm_puts(&g_vga_tm_buffer, "MACHINE_CHECK\n"); break;
            case 0x13: vga_tm_puts(&g_vga_tm_buffer, "SIMD_FP_EXCEPTION\n"); break;
            case 0x14: vga_tm_puts(&g_vga_tm_buffer, "VIRTUALIZATION_EXCEPTION\n"); break;
            case 0x15: vga_tm_puts(&g_vga_tm_buffer, "CONTROL_PROTECTION_EXCEPTION\n"); break;
            default: vga_tm_puts(&g_vga_tm_buffer, "UNKOWN\n"); break;
        }
    }

    if (p_cpu_state) {
        kprintf("[cf:%ul] ", (p_cpu_state->rflags >> 0) & 1);
        kprintf("[?:%ul] ", (p_cpu_state->rflags >> 1) & 1);
        kprintf("[pf:%ul] ", (p_cpu_state->rflags >> 2) & 1);
        kprintf("[?:%ul] ", (p_cpu_state->rflags >> 3) & 1);
        kprintf("[af:%ul] ", (p_cpu_state->rflags >> 4) & 1);
        kprintf("[?:%ul] ", (p_cpu_state->rflags >> 5) & 1);
        kprintf("[zf:%ul] ", (p_cpu_state->rflags >> 6) & 1);
        kprintf("[sf:%ul] ", (p_cpu_state->rflags >> 7) & 1);
        kprintf("[tf:%ul] ", (p_cpu_state->rflags >> 8) & 1);
        kprintf("[if:%ul] ", (p_cpu_state->rflags >> 9) & 1);
        kprintf("[df:%ul] ", (p_cpu_state->rflags >> 10) & 1);
        kprintf("[of:%ul]\n", (p_cpu_state->rflags >> 11) & 1);

        kprintf("r8= 0x%uh\n", p_cpu_state->r8);
        kprintf("r9= 0x%uh\n", p_cpu_state->r9);
        kprintf("r10=0x%uh\n", p_cpu_state->r10);
        kprintf("r11=0x%uh\n", p_cpu_state->r11);
        kprintf("r12=0x%uh\n", p_cpu_state->r12);
        kprintf("r13=0x%uh\n", p_cpu_state->r13);
        kprintf("r14=0x%uh\n", p_cpu_state->r14);
        kprintf("r15=0x%uh\n\n", p_cpu_state->r15);

        kprintf("rax=0x%uh\n", p_cpu_state->rax);
        kprintf("rcx=0x%uh\n", p_cpu_state->rcx);
        kprintf("rdx=0x%uh\n", p_cpu_state->rdx);
        kprintf("rbp=0x%uh\n", p_cpu_state->rbp);
        kprintf("rsi=0x%uh\n", p_cpu_state->rsi);
        kprintf("rdi=0x%uh\n", p_cpu_state->rdi);
        kprintf("rip=0x%uh\n", p_cpu_state->rip);
        kprintf("rsp=0x%uh\n", p_cpu_state->rsp);
    }

    while (true)
        halt();
}
