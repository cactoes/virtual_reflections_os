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

    #define HEAP_MAKE_FILTER_PARAM(p) ((void*)&p)
    #define HEAP_FILTERS_SIZE(array) (sizeof(array) / sizeof(block_filter_callback_t))

    size_t param_size = 0x100000 * 32;
    block_filter_callback_t filters[] = {
        heap_block_filters::donor_block_filter,
        heap_block_filters::unused_block_filter
    };
    heap_block_t* blocks[HEAP_FILTERS_SIZE(filters)] = {};
    heap_filter_blocks(&heap, HEAP_MAKE_FILTER_PARAM(param_size), filters, HEAP_FILTERS_SIZE(filters), blocks, HEAP_FILTERS_SIZE(filters));

    while (true) {}
}