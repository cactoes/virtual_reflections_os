#include "crash_handler.hpp"
#include "arch/generic.hpp"
#include "drivers/vga.hpp"
#include "string.hpp"
#include "utils/debug.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void __kernel_fatal(uint64_t code, const char* p_message, cpu_state_t* p_cpu_state) {
    printf(DBG, "kernel fatal triggerd: 0x%uh \"%s\"\n", code, p_message);

    vga_tm_color_map_t color {};
    color.foreground = vga_tm_color_t::WHITE;
    color.background = vga_tm_color_t::BLACK;

    vga_tm_set_color(&g_vga_tm_buffer, &color);
    vga_tm_clear_buffer(&g_vga_tm_buffer);
    vga_tm_puts(&g_vga_tm_buffer, "*** KERNEL FATAL ***\n");

    char buffer[256];
    sprintf(buffer, ARRAY_SIZE(buffer), "> ERROR CODE: 0x%uh\n", code);
    vga_tm_puts(&g_vga_tm_buffer, buffer);

    if (code >= 0 && code <= 0x15) {
        vga_tm_puts(&g_vga_tm_buffer, "> ");
        switch (code) {
            case 0xD: {
                uint16_t seg_index = p_cpu_state->error_code >> 3;
                bool is_external = p_cpu_state->error_code & 0x1;
                bool is_idt = p_cpu_state->error_code & 0x2;

                memzero(buffer, ARRAY_SIZE(buffer));
                sprintf(buffer, ARRAY_SIZE(buffer), "GENERAL_PROTECTION_FAULT\nEXTERNAL[%u] TABLE[%s] SEGMENT INDEX: [0x%uh]", is_external, is_idt ? "IDT/LDT" : "GDT", seg_index);
                vga_tm_puts(&g_vga_tm_buffer, buffer);
                break;
            }
            case 0xE:
                memzero(buffer, ARRAY_SIZE(buffer));
                sprintf(buffer, ARRAY_SIZE(buffer), "PAGE_FAULT @ 0x%uh\n%s, %s, %s, %s, %s", read_cr2(),
                (p_cpu_state->error_code & 0x1) ? "Protection Violation" : "Non-present Page",
                (p_cpu_state->error_code & 0x2) ? "Write" : "Read",
                (p_cpu_state->error_code & 0x4) ? "User" : "Kernel",
                (p_cpu_state->error_code & 0x10) ? "Instruction Fetch" : "",
                (p_cpu_state->error_code & 0x8)  ? "Reserved Bit Violation" : "");
                vga_tm_puts(&g_vga_tm_buffer, buffer);
                break;
            default:
                break;
        }
    }


    if (p_cpu_state) {
        printf(DBG, "[cf:%ul] ", (p_cpu_state->rflags >> 0) & 1);
        printf(DBG, "[?:%ul] ", (p_cpu_state->rflags >> 1) & 1);
        printf(DBG, "[pf:%ul] ", (p_cpu_state->rflags >> 2) & 1);
        printf(DBG, "[?:%ul] ", (p_cpu_state->rflags >> 3) & 1);
        printf(DBG, "[af:%ul] ", (p_cpu_state->rflags >> 4) & 1);
        printf(DBG, "[?:%ul] ", (p_cpu_state->rflags >> 5) & 1);
        printf(DBG, "[zf:%ul] ", (p_cpu_state->rflags >> 6) & 1);
        printf(DBG, "[sf:%ul] ", (p_cpu_state->rflags >> 7) & 1);
        printf(DBG, "[tf:%ul] ", (p_cpu_state->rflags >> 8) & 1);
        printf(DBG, "[if:%ul] ", (p_cpu_state->rflags >> 9) & 1);
        printf(DBG, "[df:%ul] ", (p_cpu_state->rflags >> 10) & 1);
        printf(DBG, "[of:%ul]\n", (p_cpu_state->rflags >> 11) & 1);

        printf(DBG, "r8= 0x%uh\n", p_cpu_state->r8);
        printf(DBG, "r9= 0x%uh\n", p_cpu_state->r9);
        printf(DBG, "r10=0x%uh\n", p_cpu_state->r10);
        printf(DBG, "r11=0x%uh\n", p_cpu_state->r11);
        printf(DBG, "r12=0x%uh\n", p_cpu_state->r12);
        printf(DBG, "r13=0x%uh\n", p_cpu_state->r13);
        printf(DBG, "r14=0x%uh\n", p_cpu_state->r14);
        printf(DBG, "r15=0x%uh\n\n", p_cpu_state->r15);

        printf(DBG, "rax=0x%uh\n", p_cpu_state->rax);
        printf(DBG, "rcx=0x%uh\n", p_cpu_state->rcx);
        printf(DBG, "rdx=0x%uh\n", p_cpu_state->rdx);
        printf(DBG, "rbp=0x%uh\n", p_cpu_state->rbp);
        printf(DBG, "rsi=0x%uh\n", p_cpu_state->rsi);
        printf(DBG, "rdi=0x%uh\n", p_cpu_state->rdi);
        printf(DBG, "rip=0x%uh\n", p_cpu_state->rip);
        // printf(DBG, "rsp=0x%uh\n", p_cpu_state->rsp);
    }

    while (true)
        halt();
}
