#include "libtype.h"
#include "processor/cpu_direct.h"
#include "memory/pmm.h"
#include "kbootinfo.h"
#include "memory/vmm.h"
#include "memory/generic.h"

// TODO @since 18/11/2026 -- 00:32
// abstract multiboot since its primarily on amd64

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

    void* heap_start = (void*)KERNEL_HEAP_BASE;
    void* paddr_start = pmm_alloc_pages_aligned(1 * (VMEM_PAGE_SIZE_LARGE / VMEM_PAGE_SIZE_SMALL), VMEM_PAGE_SIZE_LARGE);
    bool result = vmm_map_2mb(heap_start, paddr_start, VMEM_KERNEL_DATA);

    cpu_init();

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