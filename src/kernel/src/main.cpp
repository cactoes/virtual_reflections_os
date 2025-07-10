#include "arch/generic.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"

#include "drivers/vga.hpp"

#include "multiboot.hpp"

#include "common.hpp"

// volatile uint16_t* vga_mem = (volatile uint16_t*)0xB8000;
// memzero((void*)vga_mem, sizeof(uint16_t) * 80 * 25);
// uint8_t color = (15ul | (0ul << 4ul));
// vga_mem[0] = 'I' | (color << 8);

void* interrupt_handler(uint64_t code, void* p_rsp) {
    return p_rsp;
}

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    gdt_init();

    interrupt_set_handler(interrupt_handler);
    interrupt_init(gdt_get_kernel_code_selector());

    vga_tm_buffer_t buff {};
    vga_tm_init_buffer(&buff, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);

    vga_tm_clear_screen(&buff);

    UNUSED(mb_has_valid_magic((multiboot_t*)p_multiboot_struct));

    while (true);
}