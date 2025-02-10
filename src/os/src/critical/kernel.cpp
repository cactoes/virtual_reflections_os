#include "critical/memory.hpp"
#include "critical/kernel.hpp"
#include "driver/vga.hpp"
#include "string.hpp"

#include <stdarg.h>

void kernel_fatal_ex(uint64_t code, uint64_t extra_code, cpu_state_t* cpu_state) {
    kernel::driver::vga::tm::vga_color_map_t color {
        .foreground = kernel::driver::vga::tm::VGAC_LIGHT_GRAY,
        .background = kernel::driver::vga::tm::VGAC_BLUE,
    };

    kernel::print::set_color(&color);
    
    kernel::print::clear_screen();
    kernel::print::print("\n\n\n");
    kernel::print::print("       :(\n\n");
    kernel::print::print("       fatal error occured in kernel\n\n");
    kernel::print::print("           0x%uh", code);
    kernel::print::print("   ");

    switch (code) {
        case KFATAL_KERNEL_ASSERTION_FAILED:
            kernel::print::print("KFATAL_KERNEL_ASSERTION_FAILED");
            break;
        case KFATAL_UNHANDLED_INTERRUPT:
            kernel::print::print("KFATAL_UNHANDLED_INTERRUPT");
            break;
        case KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED:
            kernel::print::print("KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED");
            break;
        case KFATAL_PAGE_TABLE_USER_LIMIT_REACHED:
            kernel::print::print("KFATAL_PAGE_TABLE_KERNEL_LIMIT_REACHED");
            break;
        case KFATAL_PAGE_TABLE_INCORRECT_SIZE:
            kernel::print::print("KFATAL_PAGE_TABLE_INCORRECT_SIZE");
            break;
        case KFATAL_MEMORY_ALIGNMENT_INCORRECT:
            kernel::print::print("KFATAL_MEMORY_ALIGNMENT_INCORRECT");
            break;
        case KFATAL_MEMORY_OUT_OF_BOUNDS:
            kernel::print::print("KFATAL_MEMORY_OUT_OF_BOUNDS");
            break;
        default:
            kernel::print::print("KFATAL_UNKOWN");
            break;
    }

    if (extra_code != 0) {
        char buffer[20];
        size_t size = sprintf(buffer, 20, "0x%uh", extra_code);
        kernel::print::set_cusor(23 - size, 8);
        kernel::print::print(buffer);
        kernel::print::print("   ");

        switch (code) {
            case KFATAL_UNHANDLED_INTERRUPT:
                switch (extra_code) {
                    case 0x0: kernel::print::print("division by zero"); break;
                    case 0x1: kernel::print::print("single-step interrupt (see trap flag)"); break;
                    case 0x2: kernel::print::print("nmi"); break;
                    case 0x3: kernel::print::print("breakpoint (which benefits from the shorter 0xcc encoding of int 3)"); break;
                    case 0x4: kernel::print::print("overflow"); break;
                    case 0x5: kernel::print::print("bound range exceeded"); break;
                    case 0x6: kernel::print::print("invalid opcode"); break;
                    case 0x7: kernel::print::print("coprocessor not available"); break;
                    case 0x8: kernel::print::print("double fault"); break;
                    case 0x9: kernel::print::print("coprocessor segment overrun (386 or earlier only)"); break;
                    case 0xA: kernel::print::print("invalid task state segment"); break;
                    case 0xB: kernel::print::print("segment not present"); break;
                    case 0xC: kernel::print::print("stack segment fault"); break;
                    case 0xD: kernel::print::print("general protection fault"); break;
                    case 0xE: kernel::print::print("page fault"); break;
                    case 0xF: kernel::print::print("reserved"); break;
                    case 0x10: kernel::print::print("x87 floating point exception"); break;
                    case 0x11: kernel::print::print("alignment check"); break;
                    case 0x12: kernel::print::print("machine check"); break;
                    case 0x13: kernel::print::print("simd floating-point exception"); break;
                    case 0x14: kernel::print::print("virtualization exception"); break;
                    case 0x15: kernel::print::print("control protection exception (only available with cet)"); break;
                    default:
                        break;
                }
                break;
            case KFATAL_KERNEL_ASSERTION_FAILED:
                kernel::print::print("assert code");
                break;
            
            default:
                kernel::print::print("identifier");
                break;
        }
    }

    kernel::print::set_cusor(10, 10);
    kernel::print::print("cf=%ul", (cpu_state->rflags >> 0) & 1);
    kernel::print::set_cusor(10, 11);
    kernel::print::print("pf=%ul", (cpu_state->rflags >> 2) & 1);
    kernel::print::set_cusor(10, 12);
    kernel::print::print("af=%ul", (cpu_state->rflags >> 4) & 1);
    kernel::print::set_cusor(10, 13);
    kernel::print::print("zf=%ul", (cpu_state->rflags >> 6) & 1);
    kernel::print::set_cusor(10, 14);
    kernel::print::print("sf=%ul", (cpu_state->rflags >> 7) & 1);
    kernel::print::set_cusor(10, 15);
    kernel::print::print("tf=%ul", (cpu_state->rflags >> 8) & 1);
    kernel::print::set_cusor(10, 16);
    kernel::print::print("if=%ul", (cpu_state->rflags >> 9) & 1);
    kernel::print::set_cusor(10, 17);
    kernel::print::print("df=%ul", (cpu_state->rflags >> 10) & 1);
    kernel::print::set_cusor(10, 18);
    kernel::print::print("of=%ul", (cpu_state->rflags >> 11) & 1);

    kernel::print::set_cusor(25, 10);
    kernel::print::print("r8= %ul", cpu_state->r8);
    kernel::print::set_cusor(25, 11);
    kernel::print::print("r9= %ul", cpu_state->r9);
    kernel::print::set_cusor(25, 12);
    kernel::print::print("r10=%ul", cpu_state->r10);
    kernel::print::set_cusor(25, 13);
    kernel::print::print("r11=%ul", cpu_state->r11);
    kernel::print::set_cusor(25, 14);
    kernel::print::print("r12=%ul", cpu_state->r12);
    kernel::print::set_cusor(25, 15);
    kernel::print::print("r13=%ul", cpu_state->r13);
    kernel::print::set_cusor(25, 16);
    kernel::print::print("r14=%ul", cpu_state->r14);
    kernel::print::set_cusor(25, 17);
    kernel::print::print("r15=%ul", cpu_state->r15);

    kernel::print::set_cusor(40, 10);
    kernel::print::print("rax=%ul", cpu_state->rax);
    kernel::print::set_cusor(40, 11);
    kernel::print::print("rcx=%ul", cpu_state->rcx);
    kernel::print::set_cusor(40, 12);
    kernel::print::print("rdx=%ul", cpu_state->rdx);
    kernel::print::set_cusor(40, 13);
    kernel::print::print("rbp=%ul", cpu_state->rbp);
    kernel::print::set_cusor(40, 14);
    kernel::print::print("rsi=%ul", cpu_state->rsi);
    kernel::print::set_cusor(40, 15);
    kernel::print::print("rdi=%ul", cpu_state->rdi);

    kernel::print::set_cusor(VGA_TM_NUM_COLS, VGA_TM_NUM_ROWS);

    asm volatile("cli");
    while (true)
        kernel::cpu::halt();
}

void kernel::print::print(const char* fmt, ...) {
    if (fmt == nullptr || *fmt == 0)
        return;

    char buffer[512] = { 0 };

    va_list args;
    va_start(args, fmt);
    size_t strlen = sprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    (void)kernel::driver::vga::tm::print(buffer);
}

void kernel::print::print(const driver::vga::tm::vga_color_map_t* color, const char* fmt, ...) {
    if (fmt == nullptr || *fmt == 0)
        return;

    char buffer[512] = { 0 };

    va_list args;
    va_start(args, fmt);
    size_t strlen = sprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    (void)kernel::driver::vga::tm::print(color, buffer);
}

void kernel::print::clear_screen() {
    (void)kernel::driver::vga::tm::clear_screen();
}

void kernel::print::clear_row(uint32_t row) {
    (void)kernel::driver::vga::tm::clear_row(row);
}

void kernel::print::set_color(const kernel::driver::vga::tm::vga_color_map_t* color) {
    (void)kernel::driver::vga::tm::set_color(color);
}

void kernel::print::set_cusor(uint32_t x, uint32_t y) {
    (void)kernel::driver::vga::tm::set_cursor(x, y);
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
            outb(port, (uint8_t)(value & 0xFF));
            return KRESULT(0);
        case port_type_t::PT_W:
            outw(port, (uint16_t)(value & 0xFFFF));
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

    *value = 0;

    switch (type) {
        case port_type_t::PT_B:
            *(uint8_t*)value = inb(port) & 0xFF;
            return KRESULT(0);
        case port_type_t::PT_W:
            *(uint16_t*)value = inw(port) & 0xFFFF;
            return KRESULT(0);
        case port_type_t::PT_L:
            *value = inl(port);
            return KRESULT(0);
        default:
            return KRESULT(1);
    }
}