//==========================================
/// @file       amd64_syscall.h
/// @brief      
//==========================================

#pragma once

#ifndef AMD64_SYSCALL_H
#define AMD64_SYSCALL_H

#include "libdef.h"
#include "libtype.h"

typedef struct __syscall_frame {
    u64 r15, r14, r13, r12, rbp, rbx;
    u64 r10, r9, r8, rdx, rsi, rdi;

    // syscall number & return value of syscall
    u64 rax;

    // saved by the syscall
    union { u64 rip; u64 rcx; };
    union { u64 rflags; u64 r11; };

    // user stack pointer
    u64 rsp;
} __align_packed(16) syscall_frame_t;

struct __cpu;

void amd64_init_syscall(struct __cpu* cpu);

#endif // AMD64_SYSCALL_H