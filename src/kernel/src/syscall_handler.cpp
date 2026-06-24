#include "syscall_handler.hpp"
#include "io.hpp"
#include "virtual_thread.hpp"
#include "process.hpp"
#include "filesystems/vfs.hpp"
#include "time/clock.hpp"
#include "gui/display_driver.hpp"

u64 syscall_dispatch(u64 syscall_num, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6) {
    switch (syscall_num) {
        case SYSCALL_TERMINATE_PROCESS:
            return syscall_terminate_current_process();
        case SYSCALL_HEAP_ALLOC:
            return syscall_heap_alloc((size_t)a1);
        case SYSCALL_HEAP_FREE:
            return syscall_heap_free(a1);
        case SYSCALL_CREATE_WINDOW:
            return syscall_handler_create_window((window_desc_t*)a1);
        case SYSCALL_GET_WINDOW_BUFFER:
            return syscall_handler_get_window_buffer((window_handle_t)a1);
        case SYSCALL_RENDER_WINDOW:
            return syscall_handler_render_window((window_handle_t)a1);
        case SYSCALL_POLL_EVENT:
            return syscall_handler_poll_event((window_handle_t)a1, (window_event_t*)a2, (event_hook_t*)a3);
        case SYSCALL_OPEN_FILE:
            return syscall_handler_open_file((const char*)a1);
        case SYSCALL_READ_FILE:
            return syscall_handler_read_file((u64)a1, (u8**)a2, (u64*)a3);
        case SYSCALL_TIME_SINCE_BOOT:
            return syscall_handler_time_since_boot();
        case SYSCALL_WINDOW_RESIZE:
            return syscall_handler_window_resize((window_handle_t)a1, (u64)a2, (u64)a3);
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

u64 syscall_handler_create_window(window_desc_t* wnd_desc) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    // if (current_process->window)
    //     return SYSCALL_RESULT_OK;

    return wm_allocate_window(get_global_window_manager(), wnd_desc->rect.w, wnd_desc->rect.h, wnd_desc->event_hook);
}

u64 syscall_handler_get_window_buffer(window_handle_t handle) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    // if (current_process->window)
    //     return SYSCALL_RESULT_OK;

    return (u64)wm_window_get_buffer(get_global_window_manager(), handle);
}

u64 syscall_handler_render_window(window_handle_t handle) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    get_global_window_manager()->should_render = true;

    return SYSCALL_RESULT_OK;
}

u64 syscall_handler_poll_event(window_handle_t handle, window_event_t* event, event_hook_t* hook) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    if (!event)
        return SYSCALL_RESULT_OK;

    return wm_window_poll_event(get_global_window_manager(), handle, event, hook);
}

u64 syscall_handler_open_file(const char* path) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    file_descriptor_t descriptor = vfs_open_file(get_global_vfs(), path);

    return descriptor;
}

u64 syscall_handler_read_file(u64 handle, u8** data, u64* size) {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    u8* file_data = nullptr;
    size_t file_size = 0;
    if (!vfs_read_file(get_global_vfs(), handle, &file_data, &file_size))
        return SYSCALL_RESULT_ERR;

    void* target_mem = (u8*)heap_alloc(&current_process->heap, file_size);
    if (!target_mem)
        return SYSCALL_RESULT_ERR;

    memcpy(target_mem, file_data, file_size);

    *data = (u8*)target_mem;
    *size = file_size;

    return SYSCALL_RESULT_OK;
}

u64 syscall_handler_time_since_boot() {
    process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    return clock_get_time_since_boot();
}

u64 syscall_handler_window_resize(window_handle_t handle, u64 width, u64 height) {
        process_t* current_process = get_current_process();
    if (!current_process)
        return SYSCALL_RESULT_OK;

    return wm_window_resize(get_global_window_manager(), handle, width, height);
}