#include "common.hpp"
#include "vrosapi/memory.hpp"

bool syscall_create_window(u64 w, u64 h, void* buffer) {
    u64 result;

    asm volatile (
        "mov $3, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (result)
        : "r" (w), "r" (h), "r" (buffer)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return result == 0;
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

    void* buffer = syscall_malloc((window_width * window_height) * sizeof(u32));
    memzero(buffer, (window_width * window_height) * sizeof(u32));
    syscall_create_window(window_width, window_height, buffer);

    while (true)
        ;

    return 0;
}