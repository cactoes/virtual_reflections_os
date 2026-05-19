// #include "arch/x86_64/interrupt.hpp"

// extern "C" void* x86_64_isr_stub_table[];

// void*(*g_handler)(u64, void*);

// int x86_64_set_idt_entry(x86_64_idt_entry_t* p_idt, u16 kernel_code_selector, u8 int_number, void* p_handler) {
//     x86_64_idt_entry_t* descriptor = &p_idt[int_number];

//     descriptor->isr_low = (u64)p_handler & MAX_UINT16;
//     descriptor->kernel_cs = kernel_code_selector;
//     descriptor->ist = 0;
//     descriptor->attributes = 0x8E;
//     descriptor->isr_mid = ((u64)p_handler >> 16) & MAX_UINT16;
//     descriptor->isr_high = ((u64)p_handler >> 32) & MAX_UINT32;
//     descriptor->reserved = 0;

//     return 0;
// }

// void x86_64_set_idt_entries(x86_64_idt_entry_t* p_idt, u16 kernel_code_selector) {
//     for (u16 vector = 0; vector < X86_64_INT_IDT_ENTRY_COUNT; vector++)
//         x86_64_set_idt_entry(p_idt, kernel_code_selector, vector, x86_64_isr_stub_table[vector]);
// }

// void x86_64_set_idtr(x86_64_idt_register_t* p_idtr, x86_64_idt_entry_t* p_idt) {
//     p_idtr->base = (u64)&p_idt[0];
//     p_idtr->limit = (u16)(sizeof(x86_64_idt_entry_t) * X86_64_INT_IDT_ENTRY_COUNT - 1);
// }

// extern "C" void* x86_64_int_handler(u64 code, void* p_rsp) {
//     if (g_handler)
//         return g_handler(code, p_rsp);

//     return p_rsp;
// }

// extern "C" void* amd64_interrupt_dispatch(u64 code, void* rsp) {
//     return x86_64_int_handler(code, rsp);
// }

// void x86_64_set_handler(void*(p_handler)(u64, void*)) {
//     g_handler = p_handler;
// }