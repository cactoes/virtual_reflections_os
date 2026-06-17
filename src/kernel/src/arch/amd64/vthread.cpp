#include "arch/amd64/vthread.hpp"
#include "arch/amd64/idt.hpp"
#include "arch/amd64/gdt.hpp"
#include "arch/amd64/vmem.hpp"
#include "cpu.hpp"
#include "virtual_thread.hpp"
#include "process.hpp"

// defined in amd64_entry.cpp
extern amd64_tss_t* amd64_get_tss();

// defined in virtual_thread.cpp
extern void vthread_entry_point(thread_entry_t p_thread_entry);

void amd64_vthread_store_context(vthread_t* target, void* stack) {
    target->stack_top = stack;
    amd64_fpu_store(target->fpu_state);
}

void amd64_vthread_load_context(vthread_t* target) {
    amd64_set_page_table(target->page_table);
    amd64_fpu_load(target->fpu_state);

    // this means that the thread is userspace
    // & needs a kernel stack when an interrupt happens
    if (!target->parent->is_kernel_process) {
        auto kstack_top = (void*)((u64)target->kstack + VTHREAD_STACK_SIZE);
        amd64_tss_set_stack_pointer0(amd64_get_tss(), kstack_top);
        cpu_set_kernel_stack(get_current_cpu(), kstack_top);
    }
}

bool amd64_vthread_init(vthread_t* thread, void* thread_entry) {
    if (!thread || !thread_entry)
        return false;

    u64* stack = (u64*)malloc_aligned(VTHREAD_STACK_SIZE, 16);
    if (!stack)
        return false;

    memzero(stack, VTHREAD_STACK_SIZE);

    thread->fpu_state = (u8*)malloc_aligned(sizeof(u8) * 512, 16);
    if (!thread->fpu_state)
        return false;

    memzero(thread->fpu_state, 512);

    memzero(stack, VTHREAD_STACK_SIZE);
    thread->stack_bottom = stack;
    u64* stack_top = (u64*)(((u64)stack + VTHREAD_STACK_SIZE) & ~0xF);

    u64 initial_rsp = (u64)stack_top - 8;

    // itret frame
    *(--stack_top) = amd64_get_selector_for(KERNEL_DATA_SELECTOR_INDEX);
    *(--stack_top) = initial_rsp;
    *(--stack_top) = RFLAGS_IF | RFLAGS_RES;
    *(--stack_top) = amd64_get_selector_for(KERNEL_CODE_SELECTOR_INDEX);
    *(--stack_top) = (u64)vthread_entry_point;
    *(--stack_top) = 0; // error code

    // general registers
    for (int i = 0; i < 13; i++)
        *(--stack_top) = 0;

    // startup argument for the loader
    *(--stack_top) = (u64)thread_entry; // rdi

    *(--stack_top) = 0; // rbp

    thread->stack_top = stack_top;
    thread->page_table = amd64_get_page_table();

    return true;
}

bool amd64_vthread_init_main_thread(vthread_t* thread) {
    if (!thread)
        return false;

    thread->page_table = amd64_get_page_table();

    thread->fpu_state = (u8*)malloc_aligned(sizeof(u8) * 512, 16);
    if (!thread->fpu_state)
        return false;

    memzero(thread->fpu_state, 512);

    return true;
}

void amd64_vthread_cleanup(vthread_t* thread) {
    // TODO @since 22/05/2026 -- 20:15
    // free page table ?

    if (thread->fpu_state)
        free_aligned(thread->fpu_state);

    thread->fpu_state = nullptr;
}
