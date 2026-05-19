#include "syscall_handler.hpp"
#include "io.hpp"
#include "virtual_thread.hpp"

u64 syscall_dispatch(u64 syscall_num, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6) {
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

u64 syscall_terminate_current_process() {
    kprintf("[ SYSCALL ] terminated process\n");
    vthread_terminate();

    // safetey catch
    while (true);

    return SYSCALL_RESULT_OK;
}

u64 syscall_heap_alloc(size_t size) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    return (u64)heap_alloc(&current_process->heap, size);
}

u64 syscall_heap_free(void* ptr) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    heap_free(&current_process->heap, ptr);
    return SYSCALL_RESULT_OK;
}