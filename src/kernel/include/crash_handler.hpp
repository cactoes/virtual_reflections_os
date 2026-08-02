//==========================================
/// @file       crash_handler.hpp
/// @brief      kernel crash handler
//==========================================

#pragma once

#ifndef __CRASH_HANDLER_HPP__
#define __CRASH_HANDLER_HPP__

#define KERNEL_FATAL_KERNEL_EXITED                  0xFFFFFFFF
#define KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE       0xF0000000
#define KERNEL_FATAL_VMEM_INIT                      0xF0000001
#define KERNEL_FATAL_HEAP_INIT                      0xF0000002
#define KERNEL_FATAL_VTHREAD_INIT                   0xF0000003
#define KERNEL_FATAL_CRITICAL_THREAD_DIED           0xF0000004
#define KERNEL_FATAL_GRAPHICS_INIT                  0xF0000005
#define KERNEL_FATAL_PROCESS_INIT                   0xF0000006

#define KERNEL_FATAL_VTHREAD_STACK_PROTECTION       0xF0000005
#define KERNEL_FATAL_CRITICAL_SECTION_FAILED        0xF0000006

#include "common.hpp"

__noreturn
void kernel_fatal(u64 code, const char* message);

__noreturn
void kernel_crash_handler(u64 crash_code, const char* message, void* stack);

#endif // __CRASH_HANDLER_HPP__