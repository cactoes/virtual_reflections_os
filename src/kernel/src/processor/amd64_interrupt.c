#include "processor/amd64_interrupt.h"
#include "processor/amd64_gdt.h"

extern interrupt_stub_t* const isr_stub_table[AMD64_IDT_ENTRY_COUNT];
static amd64_idt_t idt;
static amd64_idtr_t idtr;

interrupt_frame_t* amd64_interrupt_stub(interrupt_frame_t* frame) {
    if (frame->vector < 32) {
        while (true)
            __asm__ volatile  ("cli; hlt");
    }

    return frame;
}

static
void idt_set_entry(amd64_idt_t* idt, u8 vector, interrupt_stub_t* handler, u16 selector, u8 ist, u8 attributes) {
    u64 address = (u64)handler;
    amd64_idt_entry_t* entry = &idt->entries[vector];

    entry->offset_low16 = (u16)address;
    entry->selector = selector;
    entry->ist = ist & 0x7;
    entry->attributes = attributes;
    entry->offset_mid16 = (u16)(address >> 16);
    entry->offset_high32 = (u32)(address >> 32);
    entry->reserved = 0;
}

static
void set_idt_entries(amd64_idt_t* idt) {
    for (u16 vector = 0; vector < AMD64_IDT_ENTRY_COUNT; vector++) {
        idt_set_entry(idt, vector, isr_stub_table[vector],
            AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_KERNEL_CODE, 0),
            0, AMD64_IDT_ATTR_PRESENT | AMD64_IDT_ATTR_RING0 | AMD64_IDT_TYPE_INTERRUPT);
    }
}

static
void update_idtr(amd64_idtr_t* idtr, const amd64_idt_t* idt) {
    idtr->base = (u64)idt;
    idtr->limit = sizeof(amd64_idt_t) - 1;
}

// CLEAN @since 19/11/2026 -- 19:46
// move this section

#define AMD64_INT_PIC1              0x20
#define AMD64_INT_PIC1_DATA         0x21
#define AMD64_INT_PIC2              0xa0
#define AMD64_INT_PIC2_DATA         0xa1
#define AMD64_INT_PIC_EOI           0x20

// and this

static inline
void amd64_out_port8(u16 port, u8 value) {
    __asm__ volatile ("outb %1, %0" : : "dN"(port), "a"(value) : "memory");
}

static inline void amd64_io_wait() { amd64_out_port8(0x80, 0); }

static
void remap_pic() {
    amd64_out_port8(AMD64_INT_PIC1, 0x11); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC2, 0x11); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC1_DATA, 0x20); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC2_DATA, 0x28); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC1_DATA, 4); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC2_DATA, 2); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC1_DATA, 1); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC2_DATA, 1); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC1_DATA, 0xFF); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC2_DATA, 0xFF); amd64_io_wait();
}

static
void install_idtr(const amd64_idtr_t* idtr) {
    __asm__ volatile(
        "lidt (%0)"
        :
        : "r"(idtr)
        : "memory"
    );
}

void amd64_cpu_idt_init() {
    set_idt_entries(&idt);
    update_idtr(&idtr, &idt);

    // todo lapic

    remap_pic();

    install_idtr(&idtr);
}