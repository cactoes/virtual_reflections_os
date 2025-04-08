#include "common.hpp"
#include "memory.hpp"
#include "vga_driver.hpp"
#include "interrupt.hpp"
#include "cpu.hpp"
#include "keyboard_driver.hpp"
#include "pit_driver.hpp"

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

[[noreturn]] void critical_fatal(uint64_t code, const char* message) {
    vga_tm_color_map_t color {
        .foreground = vga_tm_color_t::WHITE,
        .background = vga_tm_color_t::BLUE,
    };
    vga_tm_set_color(&color);
    vga_tm_clear_screen();
    vga_tm_print("(0x%uh) %s \n", code, message);
    
    while (true)
        cpu_halt();
}

cpu_state_t* handle_critical_interrupt(uint64_t code, cpu_state_t* rsp) {
    critical_fatal(code, "FATAL (critical)");
    return rsp;
}

cpu_state_t* handle_other_interrupt(uint64_t code, cpu_state_t* rsp) {
    critical_fatal(code, "FATAL (other)");
    return rsp;
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
    int_init();

    if (!vmem_init(multiboot_struct, kpml4))
        critical_fatal(0x0, "vmem_init failed");

    heap_t heap {};
    if (!heap_init(&heap, kpml4, (void*)0x40000000, 0x100000 * 32))
        critical_fatal(0x0, "heap_init failed");
    set_global_heap(&heap);

    const char spinner[] = "|/-\\";
    constexpr size_t chars_size = (sizeof(spinner) / sizeof(char)) - 1;
    size_t i = 0;
    while (true) {
        vga_tm_set_cursor(29 + 24, 20);
        vga_tm_print("%c", spinner[i++ % chars_size]);
        vga_tm_set_cursor(29 + 24, 20);
        pit_sleep(0, 200);
    }
}