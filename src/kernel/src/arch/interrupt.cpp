#include "arch/interrupt.hpp"
#include "arch/generic.hpp"

static x86_64_idt_entry_t       g_idt[X86_64_INT_IDT_ENTRY_COUNT];
static x86_64_idt_register_t    g_idtr;

bool unmask_irq(uint8_t irq_number) {
    if (irq_number >= 16)
        return false;

    const auto pic_data_port = irq_number > 8 ? X86_64_INT_PIC2_DATA : X86_64_INT_PIC1_DATA;
    irq_number = irq_number > 8 ? irq_number - 8 : irq_number;

    uint8_t mask = in_port<uint8_t>(pic_data_port);
    BIT_CLEAR(mask, irq_number);
    out_port<uint8_t>(pic_data_port, mask);

    return true;
}

void interrupt_init(uint16_t kernel_code_selector) {
    x86_64_set_idtr(&g_idtr, g_idt);
    x86_64_set_idt_entries(g_idt, kernel_code_selector);

    out_port<uint8_t>(X86_64_INT_PIC1, 0x11);
    out_port<uint8_t>(X86_64_INT_PIC2, 0x11);

    out_port<uint8_t>(X86_64_INT_PIC1_DATA, 0x20);
    out_port<uint8_t>(X86_64_INT_PIC2_DATA, 0x28);

    out_port<uint8_t>(X86_64_INT_PIC1_DATA, 4);
    out_port<uint8_t>(X86_64_INT_PIC2_DATA, 2);

    out_port<uint8_t>(X86_64_INT_PIC1_DATA, 1);
    out_port<uint8_t>(X86_64_INT_PIC2_DATA, 1);

    static const uint8_t s_all_irqs[] = {
        X86_64_INT_IRQ_PIT,
        X86_64_INT_IRQ_PS2_KEYBOARD,
        X86_64_INT_IRQ_PS2_MOUSE
    };

    for (size_t i = 0; i < ARRAY_SIZE(s_all_irqs); i++)
        UNUSED(unmask_irq(s_all_irqs[i]));

    x86_64_flush_idt(g_idtr);
    sti();
}

void interrupt_set_handler(void*(p_handler)(uint64_t, cpu_state_t*)) {
    x86_64_set_handler(p_handler);
}