#include "interrupt.hpp"
#include "cpu.hpp"

extern "C" void* isr_stub_table[];

void __flush_idt(idt_register_t idtr) {
    asm volatile ("lidt %0" : : "m"(idtr));
    asm volatile ("sti");
}

static idt_entry_t      g_idt[IDT_ENTRY_COUNT];
static idt_register_t   g_idtr;

static interrupt_callback g_int_cb_array[INT_TYPE_COUNT] {};

cpu_state_t* int_handler(uint64_t code, cpu_state_t* rsp) {
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
        case 0x15: { // end of "basic" interrupts
            const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::CRITICAL)];
            if (callback)
                rsp = callback(code, rsp);
            break;
        }
        case 0x20: { // PIT
            const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::PIT)];
            if (callback)
                rsp = callback(code, rsp);
            break;
        }
        case 0x21: { // keyboard
            const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::KEYBOARD)];
            if (callback)
                rsp = callback(code, rsp);
            break;
        }
        case 0x2C: { // mouse
            const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::MOUSE)];
            if (callback)
                rsp = callback(code, rsp);
            break;
        }
        default: {
            const auto callback = g_int_cb_array[INT_TYPE_CAST(interrupt_type::DEFAULT)];
            if (callback)
                rsp = callback(code, rsp);
            break;
        }
    }

    cpu_halt();

    return rsp;
}

void int_init() {
    g_idtr.base = (uint64_t)&g_idt[0];
    g_idtr.limit = (uint16_t)(sizeof(idt_entry_t) * IDT_ENTRY_COUNT - 1);

    for (uint8_t vector = 0; vector < 48; vector++)
        int_set_idt_entry(vector, isr_stub_table[vector], 0x8E);

    // int_set_idt_entry(0x80, (void*)systemcall, 0x8E);
    
    cpu_outb(PIC1, 0x11);
    cpu_outb(PIC2, 0x11);

    cpu_outb(PIC1_DATA, 0x20);
    cpu_outb(PIC2_DATA, 0x28);

    cpu_outb(PIC1_DATA, 4);
    cpu_outb(PIC2_DATA, 2);

    cpu_outb(PIC1_DATA, 1);
    cpu_outb(PIC2_DATA, 1);

    cpu_outb(PIC1_DATA, 0xFD);
    cpu_outb(PIC2_DATA, 0xFF);

    __flush_idt(g_idtr);

    asm volatile ("sti");
}

void int_set_idt_entry(uint8_t vector, void* handler, uint8_t flags) {
    idt_entry_t* descriptor = &g_idt[vector];

    descriptor->isr_low = (uint64_t)handler & 0xFFFF;
    descriptor->kernel_cs = 8;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->isr_mid = ((uint64_t)handler >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)handler >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

void int_set_callback(interrupt_type type, interrupt_callback int_cb) {
    g_int_cb_array[INT_TYPE_CAST(type)] = int_cb;
}
