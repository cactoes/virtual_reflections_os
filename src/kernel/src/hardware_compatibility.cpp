#include "hardware_compatibility.hpp"

using namespace hc;

//==========================================
/// @brief      section "interrupt"
//==========================================
extern "C" void* isr_stub_table[];
extern "C" cpu_state_t* __int_handler(uint64_t code, cpu_state_t* rsp);

void __flush_idt(interrupt::idt_register_t idtr) {
    asm volatile ("lidt %0" : : "m"(idtr));
    asm volatile ("sti");
}

static interrupt::idt_entry_t      g_idt[INT_IDT_ENTRY_COUNT];
static interrupt::idt_register_t   g_idtr;
static cpu_state_t*(*g_interrupt_handler)(uint64_t code, cpu_state_t* rsp) = nullptr;

int interrupt::init(cpu_state_t*(*handler)(uint64_t code, cpu_state_t* rsp), uint8_t irq_list[], size_t size) {
    g_interrupt_handler = handler;

    g_idtr.base = (uint64_t)&g_idt[0];
    g_idtr.limit = (uint16_t)(sizeof(idt_entry_t) * INT_IDT_ENTRY_COUNT - 1);

    for (uint8_t vector = 0; vector < INT_VECTOR_COUNT; vector++)
        set_idt_entry(vector, isr_stub_table[vector]);

    // int_set_idt_entry(0x80, (void*)systemcall, 0x8E);

    cpu_outb(INT_PIC1, 0x11);
    cpu_outb(INT_PIC2, 0x11);

    cpu_outb(INT_PIC1_DATA, 0x20);
    cpu_outb(INT_PIC2_DATA, 0x28);

    cpu_outb(INT_PIC1_DATA, 4);
    cpu_outb(INT_PIC2_DATA, 2);

    cpu_outb(INT_PIC1_DATA, 1);
    cpu_outb(INT_PIC2_DATA, 1);

    for (size_t i = 0; i < size; i++) {
        const auto result = unmask_irq(irq_list[i]);
        if (!result)
            return -1;
    }

    __flush_idt(g_idtr);

    asm volatile ("sti");

    return 0;
}

int interrupt::set_idt_entry(uint8_t int_number, void* handler) {
    idt_entry_t* descriptor = &g_idt[int_number];

    descriptor->isr_low = (uint64_t)handler & 0xFFFF;
    // TODO @since 19/05/2025 -- 23:23
    // gdt::get_kernel_cs()
    descriptor->kernel_cs = 8;
    descriptor->ist = 0;
    descriptor->attributes = 0x8E;
    descriptor->isr_mid = ((uint64_t)handler >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)handler >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;

    return 0;
}

int interrupt::pic_send_eoi(uint8_t irq_number) {
    if (irq_number >= 16)
        return -1;

    if (irq_number >= 8)
        cpu_outb(INT_PIC2, INT_PIC_EOI);
    
    cpu_outb(INT_PIC1, INT_PIC_EOI);
    
    return 0;
}

bool interrupt::unmask_irq(uint8_t irq_number) {
    if (irq_number >= 16)
        return false;

    const auto PIC_DATA_PORT = irq_number > 8 ? INT_PIC2_DATA : INT_PIC1_DATA;
    irq_number = irq_number > 8 ? irq_number - 8 : irq_number;

    uint8_t mask = cpu_inb(PIC_DATA_PORT);
    BIT_CLEAR(mask, irq_number);
    cpu_outb(PIC_DATA_PORT, mask);

    return true;
}

cpu_state_t* __int_handler(uint64_t code, cpu_state_t* rsp) {
    if (g_interrupt_handler)
        rsp = g_interrupt_handler(code, rsp);

    return rsp;
}

//==========================================
/// @brief      section "gdt"
//==========================================