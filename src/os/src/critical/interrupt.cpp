#include "critical/interrupt.hpp"
#include "critical/kernel.hpp"
#include "driver/keyboard.hpp"

extern "C" void* isr_stub_table[];

void __flush_idt(kernel::interrupt::idt_register_t idtr) {
    asm volatile ("lidt %0" : : "m"(idtr));
    asm volatile ("sti");
}

kernel::interrupt::idt_entry_t      g_idt[IDT_ENTRY_COUNT];
kernel::interrupt::idt_register_t   g_idtr;

kernel::driver::keyboard::keyboard_state_t g_keyboard_state {};

void systemcall(uint64_t code) {
    kernel::print::print("systemcall: %uh\n", code);
    kernel::cpu::halt();
}

void kernel::interrupt::init() {
    g_idtr.base = (uint64_t)&g_idt[0];
    g_idtr.limit = (uint16_t)(sizeof(idt_entry_t) * IDT_ENTRY_COUNT - 1);

    for (uint8_t vector = 0; vector < 48; vector++)
        set_idt_entry(vector, isr_stub_table[vector], 0x8E);

    set_idt_entry(0x80, (void*)systemcall, 0x8E);
    
    (void)cpu::out_port(cpu::PT_B, PIC1, 0x11);
    (void)cpu::out_port(cpu::PT_B, PIC2, 0x11);

    (void)cpu::out_port(cpu::PT_B, PIC1_DATA, 0x20);
    (void)cpu::out_port(cpu::PT_B, PIC2_DATA, 0x28);

    (void)cpu::out_port(cpu::PT_B, PIC1_DATA, 4);
    (void)cpu::out_port(cpu::PT_B, PIC2_DATA, 2);

    (void)cpu::out_port(cpu::PT_B, PIC1_DATA, 1);
    (void)cpu::out_port(cpu::PT_B, PIC2_DATA, 1);

    (void)cpu::out_port(cpu::PT_B, PIC1_DATA, 0xFD);
    (void)cpu::out_port(cpu::PT_B, PIC2_DATA, 0xFF);

    __flush_idt(g_idtr);

    asm volatile ("sti");
}

void kernel_interrupt_handler(uint32_t code) {
    switch (code) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
        case 0x8:
        case 0x9:
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD:
        case 0xE:
        case 0xF:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
            kernel_fatal(KFATAL_UNHANDLED_INTERRUPT, code);
            break;
        case 0x20: // PIT
            // kernel_fatal(KFATAL_UNHANDLED_INTERRUPT, code);
            break;
        case 0x21:
            kernel::driver::keyboard::handle_interrupt(&g_keyboard_state);
            kernel::print::print("pressed key");
            (void)kernel::cpu::out_port(kernel::cpu::PT_B, PIC1, 0x20);
            break;
        // case 0x2C:{ // mouse interrupt
        //     kernel::print::print("mouse int");
        //     uint32_t data;
        //     (void)kernel::cpu::in_port(kernel::cpu::PT_B, 0x60, &data);

        //     static uint8_t offset = 0;
        //     static uint8_t buffer[3] = {};

        //     if (offset == 0 && !(data & 0x08)) {
        //         offset = 0;
        //         break;
        //     }

        //     buffer[offset++] = data;

        //     if (offset >= 3) {
        //         offset = 0;
        //         kernel::print::print("Mouse: X=%i, Y=%i, Buttons=0x%uh\n",
        //             (int8_t)buffer[1],
        //             -(int8_t)buffer[2],
        //             buffer[0] & 0x07
        //         );
        //     }

        //     (void)kernel::cpu::out_port(kernel::cpu::PT_B, PIC2, 0x20);
        //     (void)kernel::cpu::out_port(kernel::cpu::PT_B, PIC1, 0x20);
        //     break;
        // }
        default:
            kernel::print::print("kernel_interrupt: %uh\n", code);
            kernel::cpu::halt();
            break;
    }
}

void kernel::interrupt::set_idt_entry(uint8_t vector, void* handler, uint8_t flags) {
    idt_entry_t* descriptor = &g_idt[vector];

    descriptor->isr_low = (uint64_t)handler & 0xFFFF;
    descriptor->kernel_cs = 8;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t)handler >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)handler >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}