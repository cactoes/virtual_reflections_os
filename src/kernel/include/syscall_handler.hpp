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

#define SYSCALL_RESULT_OK 0

#define SYSCALL_TERMINATE_PROCESS   0
#define SYSCALL_HEAP_ALLOC          1
#define SYSCALL_HEAP_FREE           2
#define SYSCALL_CREATE_WINDOW       3

#include "common.hpp"
#include "cpu.hpp"

u64 syscall_dispatch(u64 syscall_num, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6);

u64 syscall_terminate_current_process();
u64 syscall_heap_alloc(size_t size);
u64 syscall_heap_free(void* ptr);
u64 syscall_create_window(u64 width, u64 height, void* buffer);

#endif // __SYSCALL_HANDLER_HPP__