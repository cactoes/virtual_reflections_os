//==========================================
/// @file       cpu_direct.h
/// @brief      abstracted cpu functions
//==========================================

#pragma once

#ifndef CPU_DIRECT_H
#define CPU_DIRECT_H

#define CPU_ARCH_AMD64 1

#include "libtype.h"

u64 cpu_get_processor_extensions();
void cpu_enable_processor_extensions(u64 extensions);
void cpu_init();

#endif // CPU_DIRECT_H