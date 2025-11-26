//==========================================
/// @file       generic.hpp
/// @brief      generic cpu functions
//==========================================

#pragma once

#ifndef __GENERIC_HPP__
#define __GENERIC_HPP__

#include "common.hpp"

#define ARCH_X86_64
#ifdef ARCH_X86_64
#include "arch/x86_64/generic.hpp"

static inline void flush_tlb(void* p_virtual_address) {
    x86_64_flush_tlb(p_virtual_address);
}

static inline void set_pml4(void* p_ptr) {
    x86_64_set_pml4(p_ptr);
}

static inline void* get_pml4() {
    return x86_64_get_pml4();
}

static inline void set_gdt(void* p_gdtr) {
    x86_64_set_gdt(p_gdtr);
}

static inline void load_tss(uint16_t entry) {
    x86_64_load_tss(entry);
}

static inline void cli() {
    x86_64_cli();
}

static inline void sti() {
    x86_64_sti();
}

static inline void hlt() {
    x86_64_hlt();
}

NORETURN static inline void halt() {
    x86_64_halt();
}

static inline void* get_stack_pointer() {
    return x86_64_get_stack_pointer();
}

static inline int atomic_exchange(volatile int* p_ptr, int value) {
    return x86_64_atomic_exchange(p_ptr, value);
}

static inline int atomic_fetch_add(volatile int* ptr, int value) {
    return x86_64_atomic_fetch_add(ptr, value);
}

static inline int atomic_fetch_sub(volatile int* ptr, int value) {
    return x86_64_atomic_fetch_sub(ptr, value);
}

static inline void pause() {
    x86_64_pause();
}

static inline void memory() {
    x86_64_memory();
}

static inline uint64_t read_cr2() {
    return x86_64_read_cr2();
}

template <typename T>
static inline void out_port(uint16_t port, T value);

template <>
inline void out_port<uint8_t>(uint16_t port, uint8_t value) {
    x86_64_out_port<uint8_t>(port, value);
}

template <>
inline void out_port<uint16_t>(uint16_t port, uint16_t value) {
    x86_64_out_port<uint16_t>(port, value);
}

template <>
inline void out_port<uint32_t>(uint16_t port, uint32_t value) {
    x86_64_out_port<uint32_t>(port, value);
}

template <typename T>
static inline T in_port(uint16_t port);

template <>
inline uint8_t in_port<uint8_t>(uint16_t port) {
    return x86_64_in_port<uint8_t>(port);
}

template <>
inline uint16_t in_port<uint16_t>(uint16_t port) {
    return x86_64_in_port<uint16_t>(port);
}

template <>
inline uint32_t in_port<uint32_t>(uint16_t port) {
    return x86_64_in_port<uint32_t>(port);
}

static inline bool get_cpu_name(char* buffer, size_t size) {
    if (size <= 48)
        return false;
    
    memzero(buffer, size);

    uint32_t registers[4];
    char* p = buffer;
    for (uint32_t i = 0; i < 3; i++) {
        x86_64_cpuid(0x80000002 + i, 0, &registers[0], &registers[1], &registers[2], &registers[3]);
        memcpy(p, registers, sizeof(registers));
        p += sizeof(registers);
    }

    return true;
}

static inline uint64_t save_flags_and_cli() {
    return x86_64_save_flags_and_cli();
}

static inline void restore_flags(uint64_t flags) {
    x86_64_restore_flags(flags);
}

static inline void fpu_store(void* store) {
    x86_64_fpu_store(store);
}

static inline void fpu_load(void* store) {
    x86_64_fpu_load(store);
}

#endif // ARCH_X86_64

#endif // __GENERIC_HPP__
