#include "vrosapi/memory.hpp"

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

void* malloc(size_t size) noexcept {
    return syscall_malloc(size);
}

void free(void* ptr) noexcept {
    (void)syscall_free(ptr);
}

void* operator new(__SIZE_TYPE__ size) noexcept {
    return syscall_malloc(size);
}

void* operator new(__SIZE_TYPE__ size, void* ptr) noexcept {
    return ptr;
}

void* operator new[](__SIZE_TYPE__ size) noexcept {
    return syscall_malloc(size);
}

void* operator new[](__SIZE_TYPE__ size, void* ptr) noexcept {
    return ptr;
}

void operator delete(void* ptr) noexcept {
    (void)syscall_free(ptr);
}

void operator delete(void* ptr, __SIZE_TYPE__) noexcept {
    (void)syscall_free(ptr);
}

void operator delete[](void* ptr) noexcept {
    (void)syscall_free(ptr);
}

void operator delete[](void* ptr, __SIZE_TYPE__) noexcept {
    (void)syscall_free(ptr);
}
