//==========================================
/// @file       generic.hpp
/// @brief      x86_64 generic cpu functions
//==========================================

#pragma once

#ifndef __X86_64_GENERIC_HPP__
#define __X86_64_GENERIC_HPP__

#include "common.hpp"

static inline void x86_64_flush_tlb(void* p_virtual_address) {
    asm volatile("invlpg (%0)" : : "r"(p_virtual_address) : "memory");
}

static inline void x86_64_set_pml4(void* p_ptr) {
    asm volatile("mov %0, %%cr3" : : "r"(p_ptr) : "memory");
}

static inline void* x86_64_get_pml4() {
    void* p_pml4;
    asm volatile("mov %%cr3, %0" : "=r"(p_pml4) : : "memory");
    return p_pml4;
}

static inline void x86_64_set_gdt(void* p_gdtr) {
    asm volatile("lgdt (%0)" : : "r"(p_gdtr) : "memory");
}

static inline void x86_64_load_tss(uint16_t entry) {
    asm volatile(
        "mov %0, %%ax\n"
        "ltr %%ax\n"
        :
        : "r"(entry)
        : "rax", "memory"
    );
}

static inline void x86_64_cli() {
    asm volatile ("cli");
}

static inline void x86_64_sti() {
    asm volatile ("sti");
}

static inline void x86_64_hlt() {
    asm volatile ("hlt");
}

template <typename T>
static inline void x86_64_out_port(uint16_t port, T value);

template <>
inline void x86_64_out_port<uint8_t>(uint16_t port, uint8_t value) {
    asm volatile ("outb %1, %0" : : "dN"(port), "a"(value));
}

template <>
inline void x86_64_out_port<uint16_t>(uint16_t port, uint16_t value) {
    asm volatile ("outw %1, %0" : : "dN"(port), "a"(value));
}

template <>
inline void x86_64_out_port<uint32_t>(uint16_t port, uint32_t value) {
    asm volatile ("outl %1, %0" : : "dN"(port), "a"(value));
}

template <typename T>
static inline T x86_64_in_port(uint16_t port);

template <>
inline uint8_t x86_64_in_port<uint8_t>(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

template <>
inline uint16_t x86_64_in_port<uint16_t>(uint16_t port) {
    uint16_t value;
    asm volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

template <>
inline uint32_t x86_64_in_port<uint32_t>(uint16_t port) {
    uint32_t value;
    asm volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

NORETURN static inline void x86_64_halt() {
    for (;;) {
        x86_64_cli();
        x86_64_hlt();
    }
}

static inline void* x86_64_get_stack_pointer() {
    void* p_stack_pointer;
    asm volatile ("mov %%rsp, %0" : "=r" (p_stack_pointer));
    return p_stack_pointer;
}

static inline int x86_64_atomic_exchange(volatile int* p_ptr, int value) {
    int old;
    asm volatile (
        "lock xchg %0, %1"
        : "=r"(old), "+m"(*p_ptr)
        : "0"(value)
        : "memory"
    );
    return old;
}

static inline int x86_64_atomic_fetch_add(volatile int* ptr, int value) {
    asm volatile(
        "lock xadd %0, %1"
        : "+r"(value), "+m"(*ptr)
        :
        : "memory"
    );
    return value;
}

static inline int x86_64_atomic_fetch_sub(volatile int* ptr, int value) {
    value = -value;
    asm volatile(
        "lock xadd %0, %1"
        : "+r"(value), "+m"(*ptr)
        :
        : "memory"
    );
    return value;
}

static inline void x86_64_pause() {
    asm volatile("pause");
}

static inline void x86_64_memory() {
    asm volatile("" ::: "memory");
}

static inline uint64_t x86_64_read_cr2() {
    uint64_t res;
    asm volatile("mov %%cr2, %0" : "=r"(res));
    return res;
}

static inline void x86_64_cpuid(uint32_t eax_in, uint32_t ecx_in, uint32_t* eax_out, uint32_t* ebx_out, uint32_t* ecx_out, uint32_t* edx_out) {
    __asm__ volatile ("cpuid"
                : "=a"(*eax_out), "=b"(*ebx_out), "=c"(*ecx_out), "=d"(*edx_out)
                : "a"(eax_in), "c"(ecx_in));
}

static inline uint64_t x86_64_save_flags_and_cli() {
    uint64_t flags;
    asm volatile(
        "pushfq\n"
        "pop %0\n"
        "cli\n"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

static inline void x86_64_restore_flags(uint64_t flags) {
    asm volatile(
        "push %0\n"
        "popfq"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

static inline void x86_64_fpu_store(void* store) {
    asm volatile("fxsave (%0)" :: "r"(store));
}

static inline void x86_64_fpu_load(void* store) {
    asm volatile("fxrstor (%0)" :: "r"(store));
}

static inline uint64_t x86_64_rdmsr(uint32_t addr) {
    uint32_t low, high;

    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(addr)
    );

    return ((uint64_t)high << 32) | low;
}

static inline void x86_64_wrmsr(uint32_t addr, uint64_t value) {
    uint32_t low  = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(value >> 32);

    __asm__ volatile (
        "wrmsr"
        :
        : "c"(addr), "a"(low), "d"(high)
    );
}

static inline void x86_64_reload_page_table() {
    asm volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
}

#endif // __X86_64_GENERIC_HPP__