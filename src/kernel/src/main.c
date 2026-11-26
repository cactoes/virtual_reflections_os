#include "libtype.h"
#include "processor/cpu_direct.h"
#include "memory/pmm.h"
#include "kbootinfo.h"
#include "memory/vmm.h"
#include "memory/generic.h"
#include "libstring.h"
#include "io/debug.h"
#include "graphics/framebuffer.h"
#include "libdef.h"
#include "thread.h"
#include "libmem.h"

void idle_thread_loop() {
    while (true)
        __asm__ volatile ("hlt" ::: "memory");
}

__noreturn
void virtual_core_entry(cpu_t* core) {
    vthread_t idle_thread;
    vthread_create(nullptr, idle_thread_loop, &idle_thread);

    vthread_t boot_thread;
    vthread_scheduler_init_local(&boot_thread, &idle_thread);

    kdbg_printf("[core(%u)]: idle\n", core->id);

    // idle loop for now
    while (true)
        __asm__ volatile ("hlt" ::: "memory");
}

__noreturn
void virtual_kernel_entry(kboot_info_t kboot_info) {
    u64 extensions = cpu_get_processor_extensions();
    cpu_enable_processor_extensions(extensions);

    kdbg_init();

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

    kmmio_bind((void*)KERNEL_MMIO_BASE, (void*)KERNEL_MMIO_END);
    kmem_bind((void*)KERNEL_HEAP_BASE, (void*)KERNEL_HEAP_END, VMEM_PAGE_SIZE_LARGE);

    boot_cpu_init(&kboot_info);

    vthread_scheduler_init();

    vthread_t idle_thread;
    vthread_create(nullptr, idle_thread_loop, &idle_thread);

    vthread_t boot_thread;
    vthread_scheduler_init_local(&boot_thread, &idle_thread);

    cpu_enable_all_cores();

    framebuffer_init(&kboot_info.framebuffer, nullptr);
    framebuffer_clear(0xffffff);
    framebuffer_present();

    // setup process
    // setup virtual threads
    // setup virtual filsystem
    // setup pcie
    // (bind interrupts)
    // setup drivers: networking, storage, i/o
    // setup other more software related code: nic, i/o etc
    while (true);
}