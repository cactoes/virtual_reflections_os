#include "libtype.h"
#include "processor/cpu_direct.h"
#include "memory/pmm.h"
#include "kbootinfo.h"
#include "memory/vmm.h"
#include "memory/generic.h"

void virtual_kernel_entry(kboot_info_t kboot_info) {
    u64 extensions = cpu_get_processor_extensions();
    cpu_enable_processor_extensions(extensions);

    u64 physical_page_count = kboot_info.memory.size / PMM_PAGE_SIZE;
    pmm_init(physical_page_count);
    pmm_reserve_range(0, kboot_info.kernel_page_count);

    for (u64 i = 0; i < kboot_info.memory.region_count; i++) {
        memory_region_t* region = &kboot_info.memory.regions[i];
        memory_type_t type = region->addr & MEMORY_TYPE_MASK;
        if (type == MEMORY_USABLE)
            continue;

        void* address = (void*)(region->addr & ~MEMORY_TYPE_MASK);
        u64 page_count = (region->size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
        pmm_reserve_range(address, page_count);
    }

    u64 framebuffer_page_count = (kboot_info.framebuffer.pitch * kboot_info.framebuffer.height + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    void* framebuffer_start = (void*)(kboot_info.framebuffer.physical_address & ~(PMM_PAGE_SIZE - 1));
    pmm_reserve_range(framebuffer_start, framebuffer_page_count);

    cpu_init();

    // TODO @since 21/11/2026 -- 02:21
    // * do something with result
    // * store virtual heap somewhere everyone can access
    virtual_heap_t virtual_heap;
    bool result = virtual_heap_create((void*)KERNEL_HEAP_BASE, (void*)KERNEL_HEAP_END, VMEM_PAGE_SIZE_LARGE, &virtual_heap, VMEM_KERNEL_DATA);

    // initialize mmio mapper
    //      -> mmio map region 4k, ...

    // setup COM ports? or just ports in general for qemu its uart amd_COM1 on 115200 baud

    // setup process
    // setup virtual threads
    // setup virtual filsystem
    // setup pcie
    // (bind interrupts)
    // setup drivers: networking, storage, i/o
    // setup other more software related code: nic, i/o etc
    while (true);
}