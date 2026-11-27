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
#include "filesystem/vfs.h"
#include "filesystem/initramfs.h"

typedef struct __splash_image {
    u64 width, height;
    u8* data;
} splash_image_t;

static
bool load_splash(const char* path, splash_image_t* splash_image) {
    vfs_file_t* pfile;
    if (!vfs_open(path, VFS_OP_READ, &pfile))
        return false;
    
    if (vfs_read(pfile, &splash_image->width, sizeof(u64)) != sizeof(u64)) {
        vfs_close(pfile);
        return false;
    }

    if (vfs_read(pfile, &splash_image->height, sizeof(u64)) != sizeof(u64)) {
        vfs_close(pfile);
        return false;
    }

    u64 file_size = vfs_get_size(pfile);
    u64 image_data_size = splash_image->width * splash_image->height * 3;
    if (image_data_size > file_size) {
        vfs_close(pfile);
        return false;
    }

    splash_image->data = kalloc(image_data_size);
    if (!splash_image->data) {
        vfs_close(pfile);
        return false;
    }

    if (vfs_read(pfile, splash_image->data, image_data_size) != image_data_size) {
        vfs_close(pfile);
        kfree(splash_image->data);
        return false;
    }

    vfs_close(pfile);
    return true;
}

static
void start_render_splash_loop(splash_image_t* splash_image) {
    const i64 x = ((i64)framebuffer_width() - (i64)splash_image->width) / 2;
    const i64 y = ((i64)framebuffer_height() - (i64)splash_image->height) / 2;

    const char title[] = "Virtual Reflections OS";
    const i64 title_width = (sizeof(title) - 1) * 8;
    const i64 title_x = x + ((i64)splash_image->width - title_width) / 2;
    const i64 title_y = y + (i64)splash_image->height + 12;

    char booting[] = "Booting...";
    const i64 booting_width = (sizeof(booting) - 1) * 8;
    const i64 booting_x = x + ((i64)splash_image->width - booting_width) / 2;
    const i64 booting_y = title_y + 20;

    framebuffer_draw_image(x, y, splash_image->width, splash_image->height, splash_image->data);
    framebuffer_draw_text8x16(title_x, title_y, title, 0xffffff, 0x000000, false);
    framebuffer_present();

    u32 dot_count = 0;
    u64 last_update = get_local_timer_ticks();

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

bool load_ramfs(kboot_info_t* kboot_info) {
    void* initramfs_start = nullptr;
    u64 initramfs_size = 0;
        
    for (u64 i = 0; i < kboot_info->module_count; i++) {
        memory_module_t* module = &kboot_info->modules[i];
        if (!streq(module->string, "initramfs"))
            continue;

        u64 size = module->pend - module->pstart;
        void* vaddr = kmmio_map((void*)module->pstart, size);
        if (!vaddr)
            return false;

        initramfs_start = vaddr;
        initramfs_size = size;
        break;
    }

    if (!initramfs_start)
        return false;

    vfs_superblock_t* initramfs_sb = nullptr;
    if (!initramfs_create(initramfs_start, initramfs_size, &initramfs_sb))
        return false;

    return vfs_mount_root(initramfs_sb);
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

    (void*)load_ramfs(&kboot_info);

    framebuffer_init(&kboot_info.framebuffer, nullptr);
    framebuffer_clear(0x000000);
    framebuffer_present();

    kterminal_t kterm;
    kterm_init(&kterm);
    kterm_bind(&kterm);

    kterm_printf("[\x1b[90msystem\x1b[0m]: initramfs ready\n");
    kterm_printf("[\x1b[90msystem\x1b[0m]: cores -> %llu\n", get_cpu_count());

    char buffer[256];
    format_size(buffer, sizeof(buffer), kboot_info.memory.size);

    kterm_printf("[\x1b[90msystem\x1b[0m]: memory -> %s\n", buffer);

    // initial setup is done so we can kickoff the other cores if we have them
    cpu_enable_all_cores();

    // TODO @since 27/11/2026 -- 23:55
    // setup (usermode) processes
    // setup other filsystems / block device drivers
    // setup pcie
    // setup drivers: networking, storage, i/o
    // setup other more software related code: nic, i/o etc

    splash_image_t splash_image;
    if (load_splash("/etc/splash.bin", &splash_image))
        start_render_splash_loop(&splash_image);

    kterm_printf("[\x1b[90msystem\x1b[0m]: failed to load splash\n", buffer);

    // [incase the splash image doesnt work, it should to basically the same]
    // fallback to here since we need some variables n stuff from it
    // its also the entrypoint for the boot thread, so just idle if we get to this point
    while (true)
        cpu_idle();
}