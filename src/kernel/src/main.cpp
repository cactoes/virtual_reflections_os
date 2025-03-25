#include "common.hpp"
#include "memory.hpp"
#include "vga_driver.hpp"
#include "interrupt.hpp"
#include "cpu.hpp"

void critical_fatal(uint64_t code, const char* message) {
    vga_tm_color_map_t color {
        .foreground = vga_tm_color_t::WHITE,
        .background = vga_tm_color_t::BLUE,
    };
    vga_tm_set_color(&color);
    vga_tm_clear_screen();
    vga_tm_print("(%uh) %s \n", code, message);
    
    while (true)
        cpu_halt();
}

cpu_state_t* handle_critical_interrupt(uint64_t code, cpu_state_t* rsp) {
    critical_fatal(code, "FATAL");
    return rsp;
}

cpu_state_t* handle_keyboard_interrupt(uint64_t code, cpu_state_t* rsp) {
    critical_fatal(code, "FATAL");
    return rsp;
}

extern "C" void kernel_entry(multiboot_t* multiboot_struct, void* kpml4) {
    vga_tm_clear_screen();
    vga_tm_print("initializing kernel ...\n\n");

    if (multiboot_struct->magic != 0x2BADB002)
        critical_fatal(multiboot_struct->magic, "multiboot magic was invalid");

    int_set_callback(interrupt_type::CRITICAL, handle_critical_interrupt);
    int_set_callback(interrupt_type::KEYBOARD, handle_keyboard_interrupt);
    int_init();

    vmem_init(multiboot_struct, kpml4);

    heap_t heap {};
    heap_init(&heap, kpml4, (void*)0x40000000, 0x100000 * 32);
    set_global_heap(&heap);

    while (true) {}
}