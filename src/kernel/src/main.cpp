#include "common.hpp"
#include "memory.hpp"
#include "vga_driver.hpp"

void critical_fatal(uint64_t code, const char* message) {
    vga_tm_clear_screen();
    vga_tm_print("(%uh) %s \n", code, message);
    while (true) {}
}

extern "C" void kernel_entry(multiboot_t* multiboot_struct, void* kpml4) {
    vga_tm_clear_screen();
    vga_tm_print("initializing kernel ...");

    if (multiboot_struct->magic != 0x2BADB002)
        critical_fatal(multiboot_struct->magic, "multiboot magic was invalid");
    
    vmem_init(multiboot_struct, kpml4);

    heap_t heap {};
    heap_init(&heap, kpml4, (void*)0x40000000, 0x100000 * 32);
    set_global_heap(&heap);

    while (true) {}
}