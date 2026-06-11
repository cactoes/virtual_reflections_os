#include "vrosapi/file.hpp"

u64 syscall_open_file(const char* path) {
    u64 file_handle;

    asm volatile (
        "mov $7, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (file_handle)
        : "r" (path)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return file_handle;
}

bool syscall_read_file(u64 handle, u8** data, u64* size) {
    u64 result;

    asm volatile (
        "mov $8, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "mov %2, %%rsi\n\t"
        "mov %3, %%rdx\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (result)
        : "r" (handle), "r" (data), "r" (size)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return result == 0;
}