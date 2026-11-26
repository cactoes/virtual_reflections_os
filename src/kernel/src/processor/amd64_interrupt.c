#include "processor/amd64_interrupt.h"
#include "processor/amd64_gdt.h"
#include "processor/amd64_port.h"
#include "processor/amd64_apic.h"
#include "processor/amd64_tlb.h"
#include "processor/amd64_cpu.h"
#include "thread.h"
#include "processor/cpu_direct.h"
#include "crash_handler.h"
#include "libatomic.h"

extern interrupt_stub_t* const isr_stub_table[AMD64_IDT_ENTRY_COUNT];
static amd64_idt_t idt;
static amd64_idtr_t idtr;

interrupt_frame_t* amd64_interrupt_stub(interrupt_frame_t* frame) {
    const u64 vector = frame->vector;

    if (vector < 32) {
        const char* exception_text = amd64_exception_to_text(frame->vector);
        cpu_t* cpu = get_current_cpu();
        crash_handler_entry((void*)frame, exception_text, cpu->id);
    }

    if (vector == AMD64_LAPIC_SPURIOUS_VECTOR)
        return frame;

    switch (vector) {
        case AMD64_LAPIC_TIMER_VECTOR: {
            amd64_lapic_timer_handle_interrupt();
            return vthread_schedule(frame);
        }
        case AMD64_IPI_TLB_VECTOR: {
            amd64_tlb_handle_ipi();
            return frame;
        }
        case AMD64_IPI_PING_VECTOR: {
            cpu_t* cpu = get_current_cpu();

            amd64_lapic_eoi();
            atomic_add32(&cpu->ipi_ping_count, 1);
            return frame;
        }
        case AMD64_IPI_START_VECTOR: {
            cpu_t* cpu = get_current_cpu();

            amd64_lapic_eoi();
            atomic_store32(&cpu->start, 1);
            return frame;
        }
        case AMD64_IPI_KILL_VECTOR: {
            cpu_t* cpu = get_current_cpu();

            amd64_lapic_eoi();
            atomic_store32(&cpu->online, 0);
            cpu_hang();
            return frame;
        }
    }

    cpu_interrupt_handler_t handler = cpu_get_interrupt_handler(vector - AMD64_IRQ_VECTOR_BASE);
    if (handler)
        handler();

    amd64_lapic_eoi();
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
    amd64_out_port8(AMD64_INT_PIC1_DATA, 0xff); amd64_io_wait();
    amd64_out_port8(AMD64_INT_PIC2_DATA, 0xff); amd64_io_wait();
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
    remap_pic();
    install_idtr(&idtr);
}

void amd64_cpu_idt_load() {
    install_idtr(&idtr);
}

const char* amd64_exception_to_text(u64 code) {
    switch (code) {
        case AMD64_EXCEPTION_DIVISION_BY_ZERO: return "EXCEPTION_DIVISION_BY_ZERO";
        case AMD64_EXCEPTION_SINGLE_STEP_INTERRUPT: return "EXCEPTION_SINGLE_STEP_INTERRUPT";
        case AMD64_EXCEPTION_NMI: return "EXCEPTION_NMI";
        case AMD64_EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
        case AMD64_EXCEPTION_OVERFLOW: return "EXCEPTION_OVERFLOW";
        case AMD64_EXCEPTION_BOUND_RANGE_EXCEEDED: return "EXCEPTION_BOUND_RANGE_EXCEEDED";
        case AMD64_EXCEPTION_INVALID_OPCODE: return "EXCEPTION_INVALID_OPCODE";
        case AMD64_EXCEPTION_COPROCESSOR_NOT_AVAILABLE: return "EXCEPTION_COPROCESSOR_NOT_AVAILABLE";
        case AMD64_EXCEPTION_DOUBLE_FAULT: return "EXCEPTION_DOUBLE_FAULT";
        case AMD64_EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN: return "EXCEPTION_COPROCESSOR_SEGMENT_OVERRUN";
        case AMD64_EXCEPTION_INVALID_TSS: return "EXCEPTION_INVALID_TSS";
        case AMD64_EXCEPTION_SEGMENT_NOT_PRESENT: return "EXCEPTION_SEGMENT_NOT_PRESENT";
        case AMD64_EXCEPTION_STACK_SEGMENT_FAULT: return "EXCEPTION_STACK_SEGMENT_FAULT";
        case AMD64_EXCEPTION_GENERAL_PROTECTION_FAULT: return "EXCEPTION_GENERAL_PROTECTION_FAULT";
        case AMD64_EXCEPTION_PAGE_FAULT: return "EXCEPTION_PAGE_FAULT";
        case AMD64_EXCEPTION_RESERVED: return "EXCEPTION_RESERVED";
        case AMD64_EXCEPTION_X87_FLOATING_POINT_EXCEPTION: return "EXCEPTION_X87_FLOATING_POINT_EXCEPTION";
        case AMD64_EXCEPTION_ALIGNMENT_CHECK: return "EXCEPTION_ALIGNMENT_CHECK";
        case AMD64_EXCEPTION_MACHINE_CHECK: return "EXCEPTION_MACHINE_CHECK";
        case AMD64_EXCEPTION_SIMD_FP_EXCEPTION: return "EXCEPTION_SIMD_FP_EXCEPTION";
        case AMD64_EXCEPTION_VIRTUALIZATION_EXCEPTION: return "EXCEPTION_VIRTUALIZATION_EXCEPTION";
        case AMD64_EXCEPTION_CONTROL_PROTECTION_EXCEPTION: return "EXCEPTION_CONTROL_PROTECTION_EXCEPTION";
        default: return "EXCEPTION_UNKNOWN";
    }
}

u64* amd64_insert_kernel_trap_frame(u64* stack_top, void* entry) {
    u64 thread_rsp = (u64)stack_top;

    *(--stack_top) = 0;
    thread_rsp = (u64)stack_top;

    *(--stack_top) = AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_KERNEL_DATA, 0);
    *(--stack_top) = thread_rsp;
    *(--stack_top) = AMD64_RFLAGS_IF | AMD64_RFLAGS_RES;
    *(--stack_top) = AMD64_GDT_INDEX_TO_SELECTOR(GDT_INDEX_KERNEL_CODE, 0);
    *(--stack_top) = (u64)entry;

    *(--stack_top) = 0;
    *(--stack_top) = 0;

    for (u32 i = 0; i < 15; i++)
        *(--stack_top) = 0;

    return stack_top;
}