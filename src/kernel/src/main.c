#include "libtype.h"
#include "processor/cpu_direct.h"

void virtual_kernel_entry(void* multiboot2) {
    u64 extensions = cpu_get_processor_extensions();
    cpu_enable_processor_extensions(extensions);
    // setup generic init interrupt
    // setup generic other cpu setup (like gdt & msr)
    // setup memory
    // bind interrupts
    // parse system information
    // setup process
    // setup virtual threads
    // setup virtual filsystem
    // setup pcie
    // setup drivers: networking, storage, i/o
    // setup other more software related code: nic, i/o etc
    while (true);
}