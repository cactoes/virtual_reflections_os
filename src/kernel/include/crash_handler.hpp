//==========================================
/// @file       crash_handler.hpp
/// @brief      kernel crash handler
//==========================================

#pragma once

#ifndef __CRASH_HANDLER_HPP__
#define __CRASH_HANDLER_HPP__

#define KERNEL_FATAL_MULTIBOOT_MAGIC_VALIDATE       0xF0000000
#define KERNEL_FATAL_VMEM_INIT                      0xF0000001
#define KERNEL_FATAL_HEAP_INIT                      0xF0000002
#define KERNEL_FATAL_VTHREAD_INIT                   0xF0000003

#define KERNEL_FATAL_VTHREAD_STACK_PROTECTION       0xA0000000

#include "common.hpp"

// NOLINTNEXTLINE
extern "C" NORETURN void __kernel_fatal(uint64_t code, const char* p_message, cpu_state_t* p_cpu_state = nullptr);

NAKED NORETURN inline void kernel_fatal(uint64_t code, const char* p_message) {
    asm volatile (
        // reserve space for cpu_state_t
        "sub %[state_size], %%rsp\n"
        "and $-16, %%rsp\n"

        // dump the cpu state
        "mov %%rsp, %%rdi\n"
        "call x86_64_get_cpu_state\n"

        "mov %[code], %%rcx\n"
        "mov %[msg], %%r8\n"

        "mov %%rcx, %%rdi\n"
        "mov %%r8, %%rsi\n"
        "mov %%rsp, %%rdx\n"
        "call __kernel_fatal\n"

        :
        : [code] "r"(code),
          [msg] "r"(p_message),
          [state_size] "i"(sizeof(cpu_state_t))
        : "rcx", "r8", "rdi", "rsi", "rdx", "memory"
    );
}

#endif // __CRASH_HANDLER_HPP__