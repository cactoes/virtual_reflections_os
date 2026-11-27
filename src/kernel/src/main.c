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
#include "timer.h"
#include "io/kterminal.h"

extern const u64 splash_width;
extern const u64 splash_height;
extern const u8 splash_rgb[];

static void draw_splash(i64 x0, i64 y0) {
    for (u64 y = 0; y < splash_height; y++) {
        for (u64 x = 0; x < splash_width; x++) {
            u64 i = (y * splash_height + x) * 3;
            u32 color = framebuffer_color(splash_rgb[i], splash_rgb[i + 1], splash_rgb[i + 2]);
            framebuffer_put_pixel(x0 + x, y0 + y, color);
        }
    }
}

__noreturn
void idle_thread_entry() {
    while (true)
        cpu_idle();
}

__noreturn
void virtual_core_entry(cpu_t* core) {
    vthread_t idle_thread;
    vthread_create(nullptr, idle_thread_entry, &idle_thread);

    vthread_t boot_thread;
    vthread_scheduler_init_local(&boot_thread, &idle_thread);

    // idle loop for now
    while (true)
        cpu_idle();
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

    for (u64 i = 0; i < kboot_info.module_count; i++) {
        memory_module_t* module = &kboot_info.modules[i];
        u64 size = module->pend - module->pstart;
        u64 page_count = (size + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
        pmm_reserve_range((void*)module->pstart, page_count);
    }

    u64 framebuffer_page_count = (kboot_info.framebuffer.pitch * kboot_info.framebuffer.height + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    void* framebuffer_start = (void*)(kboot_info.framebuffer.physical_address & ~(PMM_PAGE_SIZE - 1));
    pmm_reserve_range(framebuffer_start, framebuffer_page_count);

    kmmio_bind((void*)KERNEL_MMIO_BASE, (void*)KERNEL_MMIO_END);
    kmem_bind((void*)KERNEL_HEAP_BASE, (void*)KERNEL_HEAP_END, VMEM_PAGE_SIZE_LARGE);

    boot_cpu_init(&kboot_info);

    vthread_scheduler_init();

    vthread_t idle_thread;
    vthread_create(nullptr, idle_thread_entry, &idle_thread);

    vthread_t boot_thread;
    vthread_scheduler_init_local(&boot_thread, &idle_thread);

    cpu_enable_all_cores();

    // setup (usermode) processes
    // setup virtual filsystem
    // setup pcie
    // setup drivers: networking, storage, i/o
    // setup other more software related code: nic, i/o etc

    framebuffer_init(&kboot_info.framebuffer, nullptr);
    framebuffer_clear(0x000000);
    framebuffer_present();

    kterminal_t kterm;
    kterm_init(&kterm);
    kterm_bind(&kterm);

    kterm_printf("[\x1b[90msystem\x1b[0m]: cores -> %llu\n", get_cpu_count());

    char buffer[256];
    format_size(buffer, sizeof(buffer), kboot_info.memory.size);

    kterm_printf("[\x1b[90msystem\x1b[0m]: memory -> %s\n", buffer);

    const i64 x = ((i64)framebuffer_width() - (i64)splash_width) / 2;
    const i64 y = ((i64)framebuffer_height() - (i64)splash_height) / 2;

    const char title[] = "Virtual Reflections OS";
    const i64 title_width = (sizeof(title) - 1) * 8;
    const i64 title_x = x + ((i64)splash_width - title_width) / 2;
    const i64 title_y = y + (i64)splash_height + 12;

    char booting[] = "Booting...";
    const i64 booting_width = (sizeof(booting) - 1) * 8;
    const i64 booting_x = x + ((i64)splash_width - booting_width) / 2;
    const i64 booting_y = title_y + 20;

    u32 dot_count = 0;
    u64 last_update = get_local_timer_ticks();

    draw_splash(x, y);
    framebuffer_draw_text8x16(title_x, title_y, title, 0xffffff, 0x000000, false);

    while (true) {
        for (u32 i = 0; i < 3; i++)
            booting[7 + i] = (i < dot_count) ? '.' : ' ';

        framebuffer_draw_text8x16(booting_x, booting_y, booting, 0x808080, 0x000000, true);
        framebuffer_present();

        while (get_local_timer_ticks() - last_update < 300)
            cpu_idle();

        last_update = get_local_timer_ticks();
        dot_count = (dot_count + 1) % 4;
    }
}