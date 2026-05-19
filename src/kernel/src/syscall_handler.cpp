#include "syscall_handler.hpp"
#include "io.hpp"
#include "virtual_thread.hpp"

uint64_t x86_64_syscall_dispatch(uint64_t syscall_num, syscall_regs_t* regs) {
    return syscall_dispatch(syscall_num,
        (void*)regs->rdi,
        (void*)regs->rsi,
        (void*)regs->rdx,
        (void*)regs->r10,
        (void*)regs->r8,
        (void*)regs->r9);
}

extern "C" uint64_t amd64_syscall_dispatch(uint64_t syscall_num, syscall_regs_t* regs) {
    return x86_64_syscall_dispatch(syscall_num, regs);
}

uint64_t syscall_dispatch(uint64_t syscall_num, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6) {
    switch (syscall_num) {
        case SYSCALL_TERMINATE_PROCESS:
            return syscall_terminate_current_process();
        case SYSCALL_HEAP_ALLOC:
            return syscall_heap_alloc((size_t)a1);
        case SYSCALL_HEAP_FREE:
            return syscall_heap_free(a1);
        default:
            break;
    }

    kprintf("[ \033[91mSYSCALL\033[0m ] unhandled syscall = %ul\n", syscall_num);
    return SYSCALL_RESULT_OK;
}

uint64_t syscall_terminate_current_process() {
    kprintf("[ SYSCALL ] terminated process\n");
    vthread_terminate();

    // safetey catch
    while (true);

    return SYSCALL_RESULT_OK;
}

uint64_t syscall_heap_alloc(size_t size) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    return (uint64_t)heap_alloc(&current_process->heap, size);
}

uint64_t syscall_heap_free(void* ptr) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    heap_free(&current_process->heap, ptr);
    return SYSCALL_RESULT_OK;
}