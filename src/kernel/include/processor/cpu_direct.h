//==========================================
/// @file       cpu_direct.h
/// @brief      abstracted cpu functions
//==========================================

#pragma once

#ifndef CPU_DIRECT_H
#define CPU_DIRECT_H

#define CPU_IRQ_TRIGGER_LEVEL   (1u << 0)
#define CPU_IRQ_POLARITY_LOW    (1u << 1)
#define CPU_IRQ_EDGE_HIGH       0u
#define CPU_IRQ_LEVEL_LOW       (CPU_IRQ_TRIGGER_LEVEL | CPU_IRQ_POLARITY_LOW)

#define CPU_ARCH_AMD64          1

#define MAX_CPUS                32
#define CPU_KERNEL_STACK_SIZE   0x4000 // 16kb, boot cpu has 2mb

#include "libtype.h"
#include "libdef.h"
#include "kbootinfo.h"

typedef void(*cpu_interrupt_handler_t)();

typedef struct __cpu cpu_t;

struct __cpu {
    u64 kernel_rsp;
    u64 user_rsp;
    struct __cpu* self;
    u32 id;
    u32 online;
    u32 start;

#if CPU_ARCHITECTURE == CPU_ARCH_AMD64
    u32 lapic_id;
    u32 ipi_ping_count;
#endif
};

u64 cpu_get_processor_extensions();
void cpu_enable_processor_extensions(u64 extensions);
void boot_cpu_init(kboot_info_t* kboot_info);
bool cpu_bind_interrupt(u32 line, u32 flags, cpu_interrupt_handler_t handler);
bool cpu_unbind_interrupt(u32 line);
cpu_interrupt_handler_t cpu_get_interrupt_handler(u32 line);
cpu_t* get_current_cpu();
void cpu_enable_all_cores();
void cpu_kill_all_cores_except(u64 core);

__noreturn
void cpu_hang();
void cpu_idle();

#endif // CPU_DIRECT_H