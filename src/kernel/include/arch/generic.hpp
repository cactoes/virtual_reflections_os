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

static inline void load_tss(u16 entry) {
    x86_64_load_tss(entry);
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

static inline u64 read_cr2() {
    return x86_64_read_cr2();
}

template <typename T>
static inline void out_port(u16 port, T value);

template <>
inline void out_port<u8>(u16 port, u8 value) {
    x86_64_out_port<u8>(port, value);
}

template <>
inline void out_port<u16>(u16 port, u16 value) {
    x86_64_out_port<u16>(port, value);
}

template <>
inline void out_port<u32>(u16 port, u32 value) {
    x86_64_out_port<u32>(port, value);
}

template <typename T>
static inline T in_port(u16 port);

template <>
inline u8 in_port<u8>(u16 port) {
    return x86_64_in_port<u8>(port);
}

template <>
inline u16 in_port<u16>(u16 port) {
    return x86_64_in_port<u16>(port);
}

template <>
inline u32 in_port<u32>(u16 port) {
    return x86_64_in_port<u32>(port);
}

static inline bool get_cpu_name(char* buffer, size_t size) {
    if (size <= 48)
        return false;
    
    memzero(buffer, size);

    u32 registers[4];
    char* p = buffer;
    for (u32 i = 0; i < 3; i++) {
        x86_64_cpuid(0x80000002 + i, 0, &registers[0], &registers[1], &registers[2], &registers[3]);
        memcpy(p, registers, sizeof(registers));
        p += sizeof(registers);
    }

    return true;
}

static inline u64 save_flags_and_cli() {
    return x86_64_save_flags_and_cli();
}

static inline void restore_flags(u64 flags) {
    x86_64_restore_flags(flags);
}

static inline void fpu_store(void* store) {
    x86_64_fpu_store(store);
}

static inline void fpu_load(void* store) {
    x86_64_fpu_load(store);
}

extern "C" void* x86_64_memset(void*, u8, size_t) noexcept;
extern "C" void* x86_64_memcpy(void*, const void*, size_t) noexcept;

inline void* memset_impl(void* dst, u8 val, size_t size) noexcept {
    return x86_64_memset(dst, val, size);
}

inline void* memzero_impl(void* dst, size_t size) noexcept {
    return x86_64_memset(dst, 0, size);
}

inline void* memcpy_impl(void* dst, const void* src, size_t size) noexcept {
    return x86_64_memcpy(dst, src, size);
}

inline bool memeq_impl(const void* a, const void* b, size_t size) noexcept {
    for (size_t i = 0; i < size; i++) {
        if (((u8*)a)[i] != ((u8*)b)[i])
            return false;
    }

    return true;
}

inline bool memreq_impl(const void* a, u8 val, size_t size) noexcept {
    for (size_t i = 0; i < size; i++) {
        if (((u8*)a)[i] != val)
            return false;
    }

    return true;
}

inline u64 rdmsr(u32 addr) {
    return x86_64_rdmsr(addr);
}

inline void wrmsr(u32 addr, u64 value) {
    x86_64_wrmsr(addr, value);
}

inline void reload_page_table() {
    x86_64_reload_page_table();
}

#endif // ARCH_X86_64

#endif // __GENERIC_HPP__
