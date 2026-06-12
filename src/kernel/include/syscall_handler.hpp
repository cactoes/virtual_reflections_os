//==========================================
/// @file       syscall_handler.hpp
/// @brief      
/// @note       syscall abi (amd64):
///             RAX = syscall number
///             RDI = arg1
///             RSI = arg2
///             RDX = arg3
///             R10 = arg4
///             R8 = arg5
///             R9 = arg6
///             RAX = return value
//==========================================

#pragma once

#ifndef __SYSCALL_HANDLER_HPP__
#define __SYSCALL_HANDLER_HPP__

#define SYSCALL_RESULT_OK   0
#define SYSCALL_RESULT_ERR  1

#define SYSCALL_TERMINATE_PROCESS   0
#define SYSCALL_HEAP_ALLOC          1
#define SYSCALL_HEAP_FREE           2
#define SYSCALL_CREATE_WINDOW       3
#define SYSCALL_GET_WINDOW_BUFFER   4
#define SYSCALL_RENDER_WINDOW       5
#define SYSCALL_POLL_EVENT          6
#define SYSCALL_OPEN_FILE           7
#define SYSCALL_READ_FILE           8
#define SYSCALL_TIME_SINCE_BOOT     9
#define SYSCALL_WINDOW_RESIZE       10

#include "common.hpp"
#include "cpu.hpp"
#include "vrosapi/window.hpp"

u64 syscall_dispatch(u64 syscall_num, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6);

u64 syscall_terminate_current_process();
u64 syscall_heap_alloc(size_t size);
u64 syscall_heap_free(void* ptr);
u64 syscall_handler_create_window(window_desc_t* wnd_desc);
u64 syscall_handler_get_window_buffer(window_handle_t handle);
u64 syscall_handler_render_window(window_handle_t handle);
u64 syscall_handler_poll_event(window_handle_t handle, window_event_t* event, event_hook_t* hook);
u64 syscall_handler_open_file(const char* path);
u64 syscall_handler_read_file(u64 handle, u8** data, u64* size);
u64 syscall_handler_time_since_boot();
u64 syscall_handler_window_resize(window_handle_t handle, u64 width, u64 height);

#endif // __SYSCALL_HANDLER_HPP__