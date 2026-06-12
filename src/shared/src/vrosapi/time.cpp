#include "vrosapi/time.hpp"

u64 syscall_time_since_boot() {
    u64 result;

    asm volatile (
        "mov $9, %%rax\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (result)
        :
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return result;
}