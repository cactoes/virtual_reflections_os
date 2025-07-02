int main(int, char**) {
    asm volatile ("int $0x80" ::: "memory");
    return 0;
}