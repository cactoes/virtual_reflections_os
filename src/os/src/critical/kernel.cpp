#include "critical/memory.hpp"
#include "critical/kernel.hpp"
#include "string.hpp"
#include <stdarg.h>

volatile uint16_t* vga_mem = (uint16_t*)KMEM_VGA_BUFFER;
uint64_t g_current_column = 0;
uint64_t g_current_row = 0;
uint8_t active_vga_color = VGAC_WHITE | VGAC_BLACK << 4;

void kernel_set_print_color(vga_color_t fg, vga_color_t bg) {
    active_vga_color = fg | (bg << 4);
}

void kernel_clear_row(uint64_t row) {
    for (uint64_t c = 0; c < NUM_COLS; c++)
        vga_mem[c + NUM_COLS * row] = ' ' | (active_vga_color << 8);
}

void kernel_clear_screen() {
    for (uint64_t r = 0; r < NUM_ROWS; r++)
        kernel_clear_row(r);

    g_current_column = 0;
    g_current_row = 0;
}

void kernel_print_new_line() {
    g_current_column = 0;
    if (g_current_row < NUM_ROWS - 1) {
        g_current_row++;
        return;
    }

    for (uint64_t r = 1; r < NUM_ROWS; r++)
        for (uint64_t c = 0; c < NUM_COLS; c++)
            vga_mem[c + NUM_COLS * (r - 1)] = vga_mem[c + NUM_COLS * r];

    kernel_clear_row(NUM_ROWS - 1);
}

void kernel_move_cursor(uint64_t row, uint64_t col) {
    int position = row * NUM_COLS + col;

    (void)kernel::cpu::out_port(kernel::cpu::PT_B, VGA_CRTC_INDEX, 14);
    (void)kernel::cpu::out_port(kernel::cpu::PT_B, VGA_CRTC_DATA, (position >> 8) & 0xFF);
    (void)kernel::cpu::out_port(kernel::cpu::PT_B, VGA_CRTC_INDEX, 15);
    (void)kernel::cpu::out_port(kernel::cpu::PT_B, VGA_CRTC_DATA, position & 0xFF);

    g_current_row = row;
    g_current_column = col;
}

void kernel_print(char ch) {
    if (g_current_column > NUM_COLS)
        kernel_print_new_line();

    switch (ch) {
        case '\n':
            kernel_print_new_line();
            break;
        default:
            vga_mem[g_current_column + NUM_COLS * g_current_row] = ch | (active_vga_color << 8);
            g_current_column++;
            break;
    }
}

void kernel_print(const char* string, ...) {
    if (string == nullptr || *string == 0)
        return;

    char buffer[256] = { 0 };

    va_list args;
    va_start(args, string);
    size_t strlen = sprintf(buffer, sizeof(buffer), string, args);
    va_end(args);

    for (uint64_t i = 0; buffer[i] != '\0'; i++) {
        const char& ch = buffer[i];
        kernel_print(ch);
    }

    kernel_move_cursor(g_current_row, g_current_column);
}

void kernel_fatal(uint64_t code, uint64_t extra_code) {
    kernel_set_print_color(vga_color_t::VGAC_LIGHT_GRAY, vga_color_t::VGAC_BLUE);
    kernel_clear_screen();
    kernel_print("\n\n\n");
    kernel_print("       :(\n\n");
    kernel_print("       fatal error occured in kernel\n\n");
    kernel_print("           0x%uh", code);
    kernel_print("   ");

    switch (code) {
        case KFATAL_KERNEL_ASSERTION_FAILED:
            kernel_print("KFATAL_KERNEL_ASSERTION_FAILED");
            break;
        case KFATAL_UNHANDLED_INTERRUPT:
            kernel_print("KFATAL_UNHANDLED_INTERRUPT");
            break;
        case KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED:
            kernel_print("KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED");
            break;
        case KFATAL_PAGE_TABLE_USER_LIMIT_REACHED:
            kernel_print("KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED");
            break;
        case KFATAL_PAGE_TABLE_INCORRECT_SIZE:
            kernel_print("KFATAL_PAGE_TABLE_INCORRECT_SIZE");
            break;
        case KFATAL_MEMORY_ALIGNMENT_INCORRECT:
            kernel_print("KFATAL_MEMORY_ALIGNMENT_INCORRECT");
            break;
        case KFATAL_MEMORY_OUT_OF_BOUNDS:
            kernel_print("KFATAL_MEMORY_OUT_OF_BOUNDS");
            break;
        default:
            kernel_print("KFATAL_UNKOWN");
            break;
    }

    if (extra_code != 0) {
        char buffer[20];
        size_t size = sprintf(buffer, 20, "0x%uh", extra_code);
        kernel_move_cursor(8, 23 - size);
        kernel_print(buffer);
        kernel_print("   ");

        switch (code) {
            case KFATAL_UNHANDLED_INTERRUPT:
                switch (extra_code) {
                    case 0x0: kernel_print("division by zero"); break;
                    case 0x1: kernel_print("single-step interrupt (see trap flag)"); break;
                    case 0x2: kernel_print("nmi"); break;
                    case 0x3: kernel_print("breakpoint (which benefits from the shorter 0xcc encoding of int 3)"); break;
                    case 0x4: kernel_print("overflow"); break;
                    case 0x5: kernel_print("bound range exceeded"); break;
                    case 0x6: kernel_print("invalid opcode"); break;
                    case 0x7: kernel_print("coprocessor not available"); break;
                    case 0x8: kernel_print("double fault"); break;
                    case 0x9: kernel_print("coprocessor segment overrun (386 or earlier only)"); break;
                    case 0xA: kernel_print("invalid task state segment"); break;
                    case 0xB: kernel_print("segment not present"); break;
                    case 0xC: kernel_print("stack segment fault"); break;
                    case 0xD: kernel_print("general protection fault"); break;
                    case 0xE: kernel_print("page fault"); break;
                    case 0xF: kernel_print("reserved"); break;
                    case 0x10: kernel_print("x87 floating point exception"); break;
                    case 0x11: kernel_print("alignment check"); break;
                    case 0x12: kernel_print("machine check"); break;
                    case 0x13: kernel_print("simd floating-point exception"); break;
                    case 0x14: kernel_print("virtualization exception"); break;
                    case 0x15: kernel_print("control protection exception (only available with cet)"); break;
                    default:
                        break;
                }
                break;
            case KFATAL_KERNEL_ASSERTION_FAILED:
                kernel_print("assert code");
                break;
            
            default:
                kernel_print("identifier");
                break;
        }
    }

    kernel_move_cursor(NUM_ROWS, NUM_COLS);

    while (true)
        kernel::cpu::halt();
}

void kernel_set_cursor(uint64_t y, uint64_t x) {
    g_current_row = y;
    g_current_column = x;
    kernel_move_cursor(y, x);
}

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" : : "dN"(port), "a"(value));
}

uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outw(uint16_t port, uint16_t value) {
    asm volatile ("outw %1, %0" : : "dN"(port), "a"(value));
}

uint16_t inw(uint16_t port) {
    uint16_t value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void outl(uint16_t port, uint32_t value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void kernel::cpu::halt() {
    for (;;) {
        asm volatile ("cli");
        asm volatile ("hlt");
    }
}

kresult_t kernel::cpu::out_port(port_type_t type, uint16_t port, uint32_t value) {
    switch (type) {
        case port_type_t::PT_B:
            outb(port, (uint8_t)value);
            return KRESULT(0);
        case port_type_t::PT_W:
            outb(port, (uint16_t)value);
            return KRESULT(0);
        case port_type_t::PT_L:
            outl(port, value);
            return KRESULT(0);
        default:
            return KRESULT(1);
    }
}

kresult_t kernel::cpu::in_port(port_type_t type, uint16_t port, uint32_t* value) {
    if (value == nullptr)
        return KRESULT(2);

    switch (type) {
        case port_type_t::PT_B:
            *(uint8_t*)value = inb(port);
            return KRESULT(0);
        case port_type_t::PT_W:
            *(uint16_t*)value = inw(port);
            return KRESULT(0);
        case port_type_t::PT_L:
            *value = inl(port);
            return KRESULT(0);
        default:
            return KRESULT(1);
    }
}