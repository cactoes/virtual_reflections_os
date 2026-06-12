#include "vrosapi/window.hpp"

window_handle_t syscall_create_window(window_desc_t* wnd_desc) {
    u64 window_handle;

    asm volatile (
        "mov $3, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (window_handle)
        : "r" (wnd_desc)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return (window_handle_t)window_handle;
}

void* syscall_get_window_buffer(window_handle_t handle) {
    u64 framebuffer;

    asm volatile (
        "mov $4, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (framebuffer)
        : "r" (handle)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return (void*)framebuffer;
}

bool syscall_render_window(window_handle_t handle) {
    u64 result;

    asm volatile (
        "mov $5, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (result)
        : "r" (handle)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return result == 0;
}

bool syscall_poll_event(window_handle_t handle, window_event_t* event, event_hook_t* hook) {
    u64 result;

    asm volatile (
        "mov $6, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (result)
        : "r" (handle), "r" (event), "r" (hook)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return result == 1;
}

bool syscall_window_resize(window_handle_t handle, u64 w, u64 h) {
    u64 result;

    asm volatile (
        "mov $10, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (result)
        : "r" (handle), "r" (w), "r" (h)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return result == 1;
}