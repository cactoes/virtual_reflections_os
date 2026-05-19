#include "common.hpp"

void* syscall_malloc(size_t size) {
    void* allocated_memory;
    asm volatile (
        "mov $1, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (allocated_memory)
        : "r" (size)
        : "rax", "rdi", "rcx", "r11", "memory"
    );
    return allocated_memory;
}

bool syscall_free(void* ptr) {
    u64 result;

    asm volatile (
        "mov $2, %%rax\n\t"
        "mov %1, %%rdi\n\t"
        "syscall\n\t"
        "mov %%rax, %0\n\t"
        : "=r" (result)
        : "r" (ptr)
        : "rax", "rdi", "rcx", "r11", "memory"
    );

    return result == 0;
}

int main() {
    int* number = (int*)syscall_malloc(sizeof(int));
    syscall_free(number);
    int* number2 = (int*)syscall_malloc(sizeof(int));

    return 0;
}