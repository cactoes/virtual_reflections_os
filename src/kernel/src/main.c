#include "libtype.h"
#include "processor/cpu_direct.h"
#include "memory/pmm.h"

// TODO @since 18/11/2026 -- 00:32
// abstract multiboot since its primarily on amd64



void virtual_kernel_entry(void* multiboot2) {
    u64 extensions = cpu_get_processor_extensions();
    cpu_enable_processor_extensions(extensions);
    
    // setup virtual memory
    pmm_init();

    cpu_init();

    // parse system information
    // setup process
    // setup virtual threads
    // setup virtual filsystem
    // setup pcie
    // (bind interrupts)
    // setup drivers: networking, storage, i/o
    // setup other more software related code: nic, i/o etc
    while (true);
}