int main() {
    unsigned long long ret;
    asm volatile(
        "mov $143, %%rax\n"
        "syscall\n"
        : "=a"(ret)
        :
        : "rcx", "r11", "memory"
    );
    
    for (int i = 0; i < 500; i += 2)
        i--;

    return 0;
}