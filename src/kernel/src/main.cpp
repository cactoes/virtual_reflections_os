typedef long unsigned int size_t;

typedef unsigned long long uint64_t;
typedef          long long int64_t;

typedef unsigned int uint32_t;
typedef          int int32_t;

typedef unsigned short uint16_t;
typedef          short int16_t;

typedef unsigned char uint8_t;
typedef   signed char int8_t;

void* memset(void* dest, uint8_t val, size_t size) {
    if (size == 0)
        return dest;

    uint8_t* p_dest = (uint8_t*)dest;

    for (size_t i = 0; i < size; i++)
        p_dest[i] = val;

    return dest;
}

void* memzero(void* dest, size_t size) {
    return memset(dest, 0, size);
}

extern "C" void kernel_entry(void* multiboot_struct, void* kpml4) {
    volatile uint16_t* vga_mem = (volatile uint16_t*)0xB8000;

    memzero((void*)vga_mem, sizeof(uint16_t) * 80 * 25);

    uint8_t color = (15ul | (0ul << 4ul));
    vga_mem[0] = 'K' | (color << 8);
    while (true);
}