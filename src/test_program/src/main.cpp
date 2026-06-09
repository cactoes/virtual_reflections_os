#include "common.hpp"
#include "vrosapi/memory.hpp"

void* syscall_create_window(u64 w, u64 h) {
    u64 framebuffer;

    asm volatile (
        "mov $3, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (framebuffer)
        : "r" (w), "r" (h)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return (void*)framebuffer;
}

int main() {
    // create inter process memory space
    // register screen buffer to kernel
    // render dekstop
    // handle mouse events
    // window management
    // syscall_free();

    u64 window_width = 400;
    u64 window_height = 400;

    void* buffer = syscall_create_window(window_width, window_height);
    memzero(buffer, (window_width * window_height) * sizeof(u32));

    while (true)
        ;

    return 0;
}