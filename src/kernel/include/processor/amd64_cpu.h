//==========================================
/// @file       amd64_cpu.h
/// @brief      am64 (x86-64) related cpu functions
//==========================================

#pragma once

#ifndef AMD64_CPU_H
#define AMD64_CPU_H

#define AMD64_CR0_MP              (1u << 1)
#define AMD64_CR0_EM              (1u << 2)
 
#define AMD64_CR4_OSFXSR          (1u << 9)
#define AMD64_CR4_OSXMMEXCPT      (1u << 10)

void amd64_enable_processor_extensions();

#endif // AMD64_CPU_H