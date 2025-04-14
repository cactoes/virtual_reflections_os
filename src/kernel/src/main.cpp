#include "common.hpp"
#include "string.hpp"
#include "memory.hpp"
#include "cpu.hpp"

#include "interrupt.hpp"
#include "virtual_thread.hpp"

#include "vga_driver.hpp"
#include "pit_driver.hpp"
#include "keyboard_driver.hpp"
#include "mouse_driver.hpp"

void draw_logo_vga_tm() {
    constexpr uint32_t x = 29;
    constexpr uint32_t y_base = 1;
    
    vga_tm_set_cursor(x, y_base + 1);  vga_tm_print("         *  ..        \n");
    vga_tm_set_cursor(x, y_base + 2);  vga_tm_print("        @@# @@.       \n");
    vga_tm_set_cursor(x, y_base + 3);  vga_tm_print("       @@*@@ @@.      \n");
    vga_tm_set_cursor(x, y_base + 4);  vga_tm_print("     .@@  #@@ @@:     \n");
    vga_tm_set_cursor(x, y_base + 5);  vga_tm_print("     @@ *@ :@@ @@:    \n");
    vga_tm_set_cursor(x, y_base + 6);  vga_tm_print("   :@@ #@%  .@@ @@=   \n");
    vga_tm_set_cursor(x, y_base + 7);  vga_tm_print("  :@@ %@#    .@@ @@+  \n");
    vga_tm_set_cursor(x, y_base + 8);  vga_tm_print(" +@@ @@*       @@ %@* \n");
    vga_tm_set_cursor(x, y_base + 9);  vga_tm_print(" @@: @@.       @@ -@@ \n");
    vga_tm_set_cursor(x, y_base + 10); vga_tm_print("  @@- @@:     @@ =@@  \n");
    vga_tm_set_cursor(x, y_base + 11); vga_tm_print("   @@* @@:   @@ *@@   \n");
    vga_tm_set_cursor(x, y_base + 12); vga_tm_print("    @@# @@. @@ *@@    \n");
    vga_tm_set_cursor(x, y_base + 13); vga_tm_print("     #@@ . @@ %@#     \n");
    vga_tm_set_cursor(x, y_base + 14); vga_tm_print("      +@@ @@ #@#      \n");
    vga_tm_set_cursor(x, y_base + 15); vga_tm_print("       -@@@ @@+       \n");
    vga_tm_set_cursor(x, y_base + 16); vga_tm_print("        :@  %=        \n");
}

extern "C" [[noreturn]] void critical_fatal_ex(uint64_t code, const char* message, cpu_state_t* cpu_state = nullptr) {
    vga_tm_color_map_t color {};
    color.foreground = vga_tm_color_t::WHITE,
    color.background = vga_tm_color_t::BLUE,

    vga_tm_set_color(&color);
    vga_tm_clear_screen();
    vga_tm_print("\n\n\n       :(\n\n");
    vga_tm_print("       fatal error occured in kernel\n");
    vga_tm_print("           (0x%uh) %s \n", code, message);

    if (cpu_state) {
        vga_tm_set_cursor(10, 10); vga_tm_print("cf=%ul", (cpu_state->rflags >> 0) & 1);
        vga_tm_set_cursor(10, 11); vga_tm_print("? =%ul", (cpu_state->rflags >> 1) & 1);
        vga_tm_set_cursor(10, 12); vga_tm_print("pf=%ul", (cpu_state->rflags >> 2) & 1);
        vga_tm_set_cursor(10, 13); vga_tm_print("? =%ul", (cpu_state->rflags >> 3) & 1);
        vga_tm_set_cursor(10, 14); vga_tm_print("af=%ul", (cpu_state->rflags >> 4) & 1);
        vga_tm_set_cursor(10, 15); vga_tm_print("? =%ul", (cpu_state->rflags >> 5) & 1);
        vga_tm_set_cursor(10, 16); vga_tm_print("zf=%ul", (cpu_state->rflags >> 6) & 1);
        vga_tm_set_cursor(10, 17); vga_tm_print("sf=%ul", (cpu_state->rflags >> 7) & 1);
        vga_tm_set_cursor(10, 18); vga_tm_print("tf=%ul", (cpu_state->rflags >> 8) & 1);
        vga_tm_set_cursor(10, 19); vga_tm_print("if=%ul", (cpu_state->rflags >> 9) & 1);
        vga_tm_set_cursor(10, 20); vga_tm_print("df=%ul", (cpu_state->rflags >> 10) & 1);
        vga_tm_set_cursor(10, 21); vga_tm_print("of=%ul", (cpu_state->rflags >> 11) & 1);
        
        vga_tm_set_cursor(20, 10); vga_tm_print("r8= 0x%uh", cpu_state->r8);
        vga_tm_set_cursor(20, 11); vga_tm_print("r9= 0x%uh", cpu_state->r9);
        vga_tm_set_cursor(20, 12); vga_tm_print("r10=0x%uh", cpu_state->r10);
        vga_tm_set_cursor(20, 13); vga_tm_print("r11=0x%uh", cpu_state->r11);
        vga_tm_set_cursor(20, 14); vga_tm_print("r12=0x%uh", cpu_state->r12);
        vga_tm_set_cursor(20, 15); vga_tm_print("r13=0x%uh", cpu_state->r13);
        vga_tm_set_cursor(20, 16); vga_tm_print("r14=0x%uh", cpu_state->r14);
        vga_tm_set_cursor(20, 17); vga_tm_print("r15=0x%uh", cpu_state->r15);

        vga_tm_set_cursor(35, 10); vga_tm_print("rax=0x%uh", cpu_state->rax);
        vga_tm_set_cursor(35, 11); vga_tm_print("rcx=0x%uh", cpu_state->rcx);
        vga_tm_set_cursor(35, 12); vga_tm_print("rdx=0x%uh", cpu_state->rdx);
        vga_tm_set_cursor(35, 13); vga_tm_print("rbp=0x%uh", cpu_state->rbp);
        vga_tm_set_cursor(35, 14); vga_tm_print("rsi=0x%uh", cpu_state->rsi);
        vga_tm_set_cursor(35, 15); vga_tm_print("rdi=0x%uh", cpu_state->rdi);
        vga_tm_set_cursor(35, 16); vga_tm_print("rip=0x%uh", cpu_state->rip);
    }
    
    while (true)
        cpu_halt();
}

__attribute__((naked)) [[noreturn]] inline void critical_fatal(uint64_t code, const char* message) {
    asm volatile (
        // Reserve space for a cpu_state_t and align the stack.
        "sub %[state_size], %%rsp\n"
        "and $-16, %%rsp\n"

        // Dump the CPU state. Pass current rsp as the state pointer.
        "mov %%rsp, %%rdi\n"
        "call __dump_cpu\n"

        // Move the incoming parameters into temporary registers.
        "mov %[code], %%rcx\n"    // Save code in RCX.
        "mov %[msg], %%r8\n"       // Save message in R8.

        // Set up the arguments for critical_fatal_ex:
        // rdi = code, rsi = message, rdx = cpu_state (rsp).
        "mov %%rcx, %%rdi\n"
        "mov %%r8, %%rsi\n"
        "mov %%rsp, %%rdx\n"
        "call critical_fatal_ex\n"

        :
        : [code] "r"(code),
          [msg] "r"(message),
          [state_size] "i"(sizeof(cpu_state_t))
        : "rcx", "r8", "rdi", "rsi", "rdx", "memory"
    );
}

cpu_state_t* handle_critical_interrupt(uint64_t code, cpu_state_t* rsp) {
    switch (code) {
        case 0x0: critical_fatal_ex(code, "FATAL: division by zero", rsp); break;
        case 0x1: critical_fatal_ex(code, "FATAL: single-step interrupt (see trap flag)", rsp); break;
        case 0x2: critical_fatal_ex(code, "FATAL: nmi", rsp); break;
        case 0x3: critical_fatal_ex(code, "FATAL: breakpoint (which benefits from the shorter 0xcc encoding of int 3)", rsp); break;
        case 0x4: critical_fatal_ex(code, "FATAL: overflow", rsp); break;
        case 0x5: critical_fatal_ex(code, "FATAL: bound range exceeded", rsp); break;
        case 0x6: critical_fatal_ex(code, "FATAL: invalid opcode", rsp); break;
        case 0x7: critical_fatal_ex(code, "FATAL: coprocessor not available", rsp); break;
        case 0x8: critical_fatal_ex(code, "FATAL: double fault", rsp); break;
        case 0x9: critical_fatal_ex(code, "FATAL: coprocessor segment overrun (386 or earlier only)", rsp); break;
        case 0xA: critical_fatal_ex(code, "FATAL: invalid task state segment", rsp); break;
        case 0xB: critical_fatal_ex(code, "FATAL: segment not present", rsp); break;
        case 0xC: critical_fatal_ex(code, "FATAL: stack segment fault", rsp); break;
        case 0xD: critical_fatal_ex(code, "FATAL: general protection fault", rsp); break;
        case 0xE: critical_fatal_ex(code, "FATAL: page fault", rsp); break;
        case 0xF: critical_fatal_ex(code, "FATAL: reserved", rsp); break;
        case 0x10: critical_fatal_ex(code, "FATAL: x87 floating point exception", rsp); break;
        case 0x11: critical_fatal_ex(code, "FATAL: alignment check", rsp); break;
        case 0x12: critical_fatal_ex(code, "FATAL: machine check", rsp); break;
        case 0x13: critical_fatal_ex(code, "FATAL: simd floating-point exception", rsp); break;
        case 0x14: critical_fatal_ex(code, "FATAL: virtualization exception", rsp); break;
        case 0x15: critical_fatal_ex(code, "FATAL: control protection exception (only available with cet)", rsp); break;
        default: critical_fatal_ex(code, "FATAL: unkown", rsp); break;
    }
    return rsp;
}

cpu_state_t* handle_other_interrupt(uint64_t code, cpu_state_t* rsp) {
    critical_fatal_ex(code, "FATAL (other)", rsp);
    return rsp;
}

void task_2_test() {
    vga_tm_print("task2");

    while (true) {}
}

extern "C" void kernel_entry(multiboot_t* multiboot_struct, void* kpml4) {
    vga_tm_clear_screen();
    draw_logo_vga_tm();
    vga_tm_set_cursor(29, 20);
    vga_tm_print("initializing kernel ...");

    if (multiboot_struct->magic != 0x2badb002)
        critical_fatal(multiboot_struct->magic, "multiboot magic was invalid");

    key_state_t key_states[0xff] = {};
    keyboard_init(key_states);

    pit_timer_t timers[1] = {
        {
            // main process
            .id = 0,
            .tick = 0
        }
    };
    pit_init(timers, 1);

    int_set_callback(interrupt_type::OTHER, handle_other_interrupt);
    int_set_callback(interrupt_type::CRITICAL, handle_critical_interrupt);
    int_set_callback(interrupt_type::KEYBOARD, keyboard_handle_interrupt);
    int_set_callback(interrupt_type::PIT, pit_handle_interrupt);
    int_set_callback(interrupt_type::MOUSE, mouse_handle_interrupt);
    int_init();

    mouse_state_t mouse_state = {};
    ps2_mouse_init(&mouse_state);

    if (!vmem_init(multiboot_struct, kpml4))
        critical_fatal(0x0, "vmem_init failed");

    heap_t heap {};
    if (!heap_init(&heap, kpml4, (void*)0x40000000, 0x100000 * 32))
        critical_fatal(0x0, "heap_init failed");
    set_global_heap(&heap);

    vthread_t main_thread {};
    vthread_add(&main_thread);

    // FIXME @since 14/04/2025 -- 01:04
    // shit doesnt work always get int13
    vthread_t thread_2 {};
    vthread_create(&thread_2, task_2_test);

    const char* spinner[] = { ".  ", ".. ", "...", ".. ", ".  ", "   " };
    constexpr size_t chars_size = (sizeof(spinner) / sizeof(char*));
    size_t i = 0;
    while (true) {
        vga_tm_set_cursor(49, 20);
        vga_tm_print("%s", spinner[i++ % chars_size]);
        vga_tm_set_cursor(VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
        pit_sleep(0, 250);
    }
}