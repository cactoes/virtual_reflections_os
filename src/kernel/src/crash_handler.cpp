#include "crash_handler.hpp"

// #include "drivers/vga.hpp"
// #include "std/string.hpp"
// #include "io.hpp"
// #include "utils/mutex.hpp"
// #include "arch/amd64/cpu.hpp"

#include "io.hpp"
#include "interrupt_manager.hpp"
#include "virtual_thread.hpp"
#include "arch/arch_selector.hpp"
#include "utils/debug.hpp"
#include "drivers/graphics/graphics_driver.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64
extern "C" void amd64_crash_dump(u64 code, const char* message, struct interrupt_regs_t* stack);
#endif

void kernel_fatal(u64 code, const char* message) {
    kprintf("kernel encountered an unhandleable situation (0x%uh): \"%s\"\n    ", code, message);

    switch (code) {
        case KERNEL_FATAL_KERNEL_EXITED: kprintf("KERNEL_FATAL_KERNEL_EXITED"); break;
        case KERNEL_FATAL_CRITICAL_THREAD_DIED: kprintf("KERNEL_FATAL_CRITICAL_THREAD_DIED"); break;
        case KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE: kprintf("KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE"); break;
        case KERNEL_FATAL_VMEM_INIT: kprintf("KERNEL_FATAL_VMEM_INIT"); break;
        case KERNEL_FATAL_HEAP_INIT: kprintf("KERNEL_FATAL_HEAP_INIT"); break;
        case KERNEL_FATAL_VTHREAD_INIT: kprintf("KERNEL_FATAL_VTHREAD_INIT"); break;
        case KERNEL_FATAL_VTHREAD_STACK_PROTECTION: kprintf("KERNEL_FATAL_VTHREAD_STACK_PROTECTION"); break;
        default: kprintf("KERNEL_FATAL_UNKNOWN"); break;
    }

    kprintf("\n");

    graphics_driver_t* gd = get_global_graphics_driver();

    size_t w, h;
    graphics_driver_get_size(gd, &w, &h);
    graphics_driver_draw_square(gd, 0, 0, w, h, { 0, 0, 0 });

    size_t w2, h2;
    graphics_driver_get_text_size(gd, " Unrecoverable Exception ", &w2, &h2);
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100, " Unrecoverable Exception ", { 0, 0, 0 }, { 255, 255, 255 });
    
    // we take this since its the longes piece of text
    graphics_driver_get_text_size(gd, "The system encounterd an unhandleable situation", &w2, &h2);

    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 2, "The system encounterd an unhandleable situation", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 3, "Recovery is not possible.", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 6, "A system reboot is required.", { 255, 255, 255 });

    char buffer[256] {};
    sprintf(buffer, 256, "Stop code: 0x%uh", code);
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 8, buffer, { 255, 255, 255 });

    graphics_driver_render(gd);

    // reboot here?

    while (true)
        debug_trap("kernel fatal");
}

void kernel_crash_handler(u64 crash_code, const char* message, void* stack) {
    disable_interrupts();

    kprintf("kernel crash handler triggered (0x%uh): \"%s\"\n", crash_code, message);

#if CPU_ARCHITECTURE == ARCH_AMD64
    amd64_crash_dump(crash_code, message, (struct interrupt_regs_t*)stack);
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif

    // if not main thread just terminate the thread not the system
    // & if a valid tls is setup
    if (__thread_tls && __thread_tls->handle != VTHREAD_MAIN_THREAD_HANDLE && __thread_tls->handle != VTHREAD_HANDLE_INVALID) {
        kprintf("[thread %ul]: terminated (crashed or forcefully stopped)\n", __thread_tls->handle);
        mutex_clear_all_thread_references_and_release(__thread_tls->handle);

        // to prevent crash looping
        if (vthread_get_state() != vthread_state_t::STOPPING)
            vthread_terminate();

        while (true)
            ;
    }

    graphics_driver_t* gd = get_global_graphics_driver();

    size_t w, h;
    graphics_driver_get_size(gd, &w, &h);
    graphics_driver_draw_square(gd, 0, 0, w, h, { 0, 0, 0 });

    size_t w2, h2;
    graphics_driver_get_text_size(gd, " Corrupted Beyond Repair ", &w2, &h2);
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100, " Corrupted Beyond Repair ", { 0, 0, 0 }, { 255, 255, 255 });
    
    // we take this since its the longes piece of text
    graphics_driver_get_text_size(gd, "Additional diagnostic information generated (if available).", &w2, &h2);

    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 2, "The system encounterd a critical error.", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 3, "Immediate recovery is not possible.", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 5, "Additional diagnostic information generated (if available).", { 255, 255, 255 });
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 6, "A system reboot is required.", { 255, 255, 255 });

    char buffer[256] {};
    sprintf(buffer, 256, "Stop code: 0x%uh", crash_code);
    graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 8, buffer, { 255, 255, 255 });

    graphics_driver_render(gd);

    // reboot after key press?

    while (true)
        debug_trap("kernel crash");
}

//     if (code == 0xe) {
//         bool is_reason_protection = (cpu_state->error_code & 0x1);
//         bool is_operation_write = (cpu_state->error_code & 0x2);
//         bool is_mode_user = (cpu_state->error_code & 0x4);

//         char buffer[256] {};
//         sprintf(buffer, 256, "address: 0x%p", amd64_read_cr2());
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 9, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "reason: %s", is_reason_protection ? "protection violation" : "non-present page");
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 10, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "operation: %s", is_operation_write ? "write" : "read");
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 11, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "mode: %s", is_mode_user ? "user" : "kernel");
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 12, buffer, { 255, 255, 255 });
//     } else if (code == 0xd) {
//         char buffer[256] {};
//         sprintf(buffer, 256, "rsp: 0x%p [% 16 == %ul]", cpu_state->rsp, (u64)(cpu_state->rsp % 16));
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 9, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "rbp: 0x%p [% 16 == %ul]", cpu_state->rbp, (u64)(cpu_state->rbp % 16));
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 10, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "rip: 0x%p [% 16 == %ul]", cpu_state->rip, (u64)(cpu_state->rip % 16));
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 11, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "error code: %ul", cpu_state->error_code);
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 12, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "cr3: %ul", amd64_read_cr3());
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 13, buffer, { 255, 255, 255 });

//         sprintf(buffer, 256, "handle: %ul", temp_handle);
//         graphics_driver_draw_text(gd, w / 2 - w2 / 2, 100 + h2 * 14, buffer, { 255, 255, 255 });
//     }