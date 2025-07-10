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
            return 1;
    }

    __flush_idt(g_idtr);

    asm volatile ("sti");

    return 0;
}

int interrupt::set_idt_entry(uint8_t int_number, void* handler) {
    idt_entry_t* descriptor = &g_idt[int_number];

    descriptor->isr_low = (uint64_t)handler & 0xFFFF;
    descriptor->kernel_cs = (uint16_t)gdt_tss::get_kernel_code_selector();
    descriptor->ist = 0;
    descriptor->attributes = 0x8E;
    descriptor->isr_mid = ((uint64_t)handler >> 16) & 0xFFFF;
    descriptor->isr_high = ((uint64_t)handler >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;

    return 0;
}

int interrupt::pic_send_eoi(uint8_t irq_number) {
    if (irq_number >= 16)
        return 1;

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
/// @brief      section "gdt_tss"
//==========================================

void __set_gdt(void* gdtr) {
    asm volatile("lgdt (%0)" : : "r"(gdtr) : "memory");
}

void __load_tss(uint64_t entry) {
    asm volatile(
        "mov %0, %%ax\n"
        "ltr %%ax\n"
        :
        : "r"((uint16_t)entry)
        : "rax", "memory"
    );
}

static gdt_tss::gdt_t   g_gdt;
static gdt_tss::gdtr_t  g_gdtr;
static gdt_tss::tss_t   g_tss;

int gdt_tss::init() {
    // 0x0
    memzero(&g_gdt.entries[GDT_INDEX_NULL], sizeof(gdt_entry_t));

    // kernel code 64 0x8
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].limit = 0;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].base_low16 = 0;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].base_mid8 = 0;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].access = GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_SEGMENT | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_READWRITE;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].granularity = GDT_LONG_MODE;
    g_gdt.entries[GDT_INDEX_KERNEL_64_CODE].base_high8 = 0;

    // tss entry always last entry
    g_gdt.tss_entry.length = sizeof(tss_t);
    g_gdt.tss_entry.base_low16 = (uint16_t)(((uint64_t)&g_tss) & 0xffff);
    g_gdt.tss_entry.base_mid8 = (uint8_t)(((uint64_t)&g_tss >> 16) & 0xff);
    g_gdt.tss_entry.flags1 = 0b10001001;
    g_gdt.tss_entry.flags2 = 0;
    g_gdt.tss_entry.base_high8 = (uint8_t)(((uint64_t)&g_tss >> 24) & 0xff);
    g_gdt.tss_entry.base_upper32 = (uint32_t) (((uint64_t)&g_tss >> 32) & 0xffffffff);
    g_gdt.tss_entry.reserved = 0;

    g_gdtr.limit = sizeof(g_gdt) - 1;
    g_gdtr.address = (uint64_t)&g_gdt;

    __set_gdt(&g_gdtr);

    memzero(&g_tss, sizeof(tss_t));

    __load_tss(GDT_INDEX_TO_ENTRY(GDT_INDEX_TSS));

    return 0;
}

int gdt_tss::set_stack_pointer0(void* stack_pointer) {
    g_tss.rsp0 = (uint64_t)stack_pointer;
    return 0;
}

uint64_t gdt_tss::get_kernel_code_selector() {
    return GDT_INDEX_TO_ENTRY(GDT_INDEX_KERNEL_64_CODE);
}

//==========================================
/// @brief      section "pit"
//==========================================

int pit::init(uint64_t times_per_s) {

    cpu_outb(0x43, 0x36);
    uint64_t count = 1193182 / times_per_s;

    cpu_outb(0x40, (uint8_t)(count & 0xFF));
    cpu_outb(0x40, (uint8_t)((count >> 8) & 0xFF));

    return 0;
}

uint64_t pit::read() {
    uint32_t count = 0;

    asm volatile ("cli");

    cpu_outb(0x43, 0x00);
    count = cpu_inb(0x40);
    count |= cpu_inb(0x40) << 8;

    asm volatile ("sti");

    return count;
}