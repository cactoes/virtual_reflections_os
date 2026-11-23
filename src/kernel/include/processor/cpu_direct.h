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

#define CPU_ARCH_AMD64      1

#define MAX_CPUS            32

#include "libtype.h"
#include "kbootinfo.h"

typedef void(*cpu_interrupt_handler_t)();

typedef struct __cpu {
    u64 kernel_rsp;
    u64 user_rsp;
    struct __cpu* self;
    u32 id;
    u32 lapic_id;
} cpu_t;

u64 cpu_get_processor_extensions();
void cpu_enable_processor_extensions(u64 extensions);
void boot_cpu_init(kboot_info_t* kboot_info);
bool cpu_bind_interrupt(u32 line, u32 flags, cpu_interrupt_handler_t handler);
bool cpu_unbind_interrupt(u32 line);
cpu_interrupt_handler_t cpu_get_interrupt_handler(u32 line);
cpu_t* get_current_cpu();

#endif // CPU_DIRECT_H