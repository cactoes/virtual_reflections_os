/** syscall format
    RAX = syscall number
    RDI = arg1
    RSI = arg2
    RDX = arg3
    R10 = arg4
    R8 = arg5
    R9 = arg6
    RAX = return value
*/

void* syscall_malloc(unsigned long long size) {
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

int main() {
    int* number = (int*)syscall_malloc(sizeof(int));

    *number = 10;

    if (*number == 10) {
        unsigned long long ret;
        asm volatile(
            "mov $10, %%rax\n"
            "syscall\n"
            : "=a"(ret)
            :
            : "rcx", "r11", "memory"
        );
    } else {
        unsigned long long ret;
        asm volatile(
            "mov $100, %%rax\n"
            "syscall\n"
            : "=a"(ret)
            :
            : "rcx", "r11", "memory"
        );
    }

    return 0;
}