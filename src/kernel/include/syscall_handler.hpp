//==========================================
/// @file       syscall_handler.hpp
/// @brief      
/// @note       syscall abi:
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

#define SYSCALL_TERMINATE_PROCESS 0
#define SYSCALL_HEAP_ALLOC 1
#define SYSCALL_HEAP_FREE 2

#include "common.hpp"
#include "cpu.hpp"

extern "C" uint64_t x86_64_syscall_dispatch(uint64_t syscall_num, syscall_regs_t* regs);
uint64_t syscall_dispatch(uint64_t syscall_num, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6);

uint64_t syscall_terminate_current_process();
uint64_t syscall_heap_alloc(size_t size);
uint64_t syscall_heap_free(void* ptr);

#endif // __SYSCALL_HANDLER_HPP__