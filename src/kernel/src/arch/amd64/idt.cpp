#include "arch/amd64/idt.hpp"
#include "arch/amd64/port.hpp"

extern "C" void* amd64_isr_stub_table[];

static void amd64_set_idt_entry(amd64_idt_entry_t* idt, u16 kernel_code_selector, u8 int_number, void* callback) {
    amd64_idt_entry_t* descriptor = &idt[int_number];

    descriptor->isr_low = (u64)callback & MAX_UINT16;
    descriptor->kernel_cs = kernel_code_selector;
    descriptor->ist = 0;
    descriptor->attributes = 0x8E;
    descriptor->isr_mid = ((u64)callback >> 16) & MAX_UINT16;
    descriptor->isr_high = ((u64)callback >> 32) & MAX_UINT32;
    descriptor->reserved = 0;
}

void amd64_set_idt_entries(amd64_idt_entry_t* p_idt, u16 kernel_code_selector) {
    for (u16 vector = 0; vector < AMD64_INT_IDT_ENTRY_COUNT; vector++)
        amd64_set_idt_entry(p_idt, kernel_code_selector, vector, amd64_isr_stub_table[vector]);
}

void amd64_set_idtr(amd64_idt_register_t* idtr, amd64_idt_entry_t* idt) {
    idtr->base = (u64)&idt[0];
    idtr->limit = (u16)(sizeof(amd64_idt_entry_t) * AMD64_INT_IDT_ENTRY_COUNT - 1);
}

void amd64_interrupt_send_eoi(u8 irq) {
    if (irq >= 16)
        return;

    if (irq >= 8)
        amd64_out_port8(AMD64_INT_PIC2, AMD64_INT_PIC_EOI);
    
    amd64_out_port8(AMD64_INT_PIC1, AMD64_INT_PIC_EOI);
}

void amd64_irq_unmask(u8 irq) {
    if (irq >= 16)
        return;

    const auto pic_data_port = irq >= 8 ? AMD64_INT_PIC2_DATA : AMD64_INT_PIC1_DATA;
    irq = irq >= 8 ? irq - 8 : irq;

    u8 mask = amd64_in_port8(pic_data_port);
    BIT_CLEAR(mask, irq);
    amd64_out_port8(pic_data_port, mask);
}