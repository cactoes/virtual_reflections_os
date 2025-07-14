#include "arch/generic.hpp"
#include "arch/gdt.hpp"
#include "arch/interrupt.hpp"

#include "drivers/vga.hpp"

#include "memory/vmem.hpp"
#include "memory/heap.hpp"

#include "utils/debug.hpp"

#include "multiboot.hpp"
#include "string.hpp"
#include "common.hpp"
#include "crash_handler.hpp"

enum print_mode_t {
    STD,
    DBG
};

void printf(print_mode_t mode, const char* p_str, ...) {
    char buffer[256] = { 0 };

    va_list args;
    va_start(args, p_str);
    size_t strlen = sprintf(buffer, (unsigned long int)sizeof(buffer), p_str, args);
    va_end(args);

    switch (mode) {
        case DBG:
            debug_puts(buffer);
            break;
        case STD:
        default:
            vga_tm_puts(&g_vga_tm_buffer, buffer);
            break;
    }
}


void* interrupt_handler(uint64_t code, cpu_state_t* p_rsp) {
    if (code >= 0 && code <= 0x15) {
        __kernel_fatal(code, "", p_rsp);

    }
    return p_rsp;
}

extern "C" void kernel_entry(void* p_multiboot_struct, void* p_kpml4) {
    // validate multiboot
    UNUSED(mb_has_valid_magic((multiboot_t*)p_multiboot_struct));

    // initialize the gdt / tss
    gdt_init();

    // initialze the interrupt line(s)
    interrupt_set_handler(interrupt_handler);
    interrupt_init(gdt_get_kernel_code_selector());

    // initialze vga text mode
    vga_tm_init_buffer(&g_vga_tm_buffer, (void*)VGA_TM_BUFFER_ADDR, VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);
    vga_tm_clear_buffer(&g_vga_tm_buffer);

    // initialze the debug out stream
    debug_init();

    // initialze virtual memory
    UNUSED(vmem_init(p_multiboot_struct, p_kpml4));
    vmem_identity_map(p_kpml4);
    set_pml4(p_kpml4);

    // initialze the global heap
    heap_t heap {};
    UNUSED(heap_init(&heap, p_kpml4, (void*)VMEM_HEAP_START_ADDR, 0x100000 * 32));
    set_global_heap(&heap);

    // interrupt stuff

    // pci(e)

    // setup vfs

    // threads / processes

    // kernel finished
    // printf(STD, "Kernel finished initializing, press a key to start the terminal ...\n");
    // printf(DBG, "Kernel finished initializing\n");

    volatile int* ptr = (int*)0x1234564478;
    int val = *ptr;
    // asm volatile (
    //     "mov $0xFFFF, %%ax\n"   // Invalid selector (not present in GDT)
    //     "ltr %%ax\n"            // Load to Task Register — will trigger #GP
    //     :
    //     :
    //     : "rax"
    // );

    // we shoudn t reach this point since the kernel should never stop
    // incase we do just hang here so we dont break anything
    while (true);
}