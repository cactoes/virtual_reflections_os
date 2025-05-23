//==========================================
/// @file       virtual_thread.hpp
/// @brief      very basic implementation of multitasking
///  TODO       vthread cleanup
///             wrapper start function
///             better thread hander / scheduler
//==========================================

#pragma once

#ifndef __VIRTUAL_THREAD_HPP__
#define __VIRTUAL_THREAD_HPP__

#define VTHREAD_STACK_SIZE  8192         // 8KB
#define VTHREAD_MAX_COUNT   256

#define VTHREAD_TLS_ENTRY_COUNT     64

#include "cpu.hpp"
#include "memory.hpp"

struct tls_base_t {
    uint64_t vtid;
    uint64_t sleep_until_tick;
};

enum class vthread_state_t {
    UNKNOWN = 0,
    STARTING,
    RUNNING,
    SLEEPING,
    STOPPING
};

struct vthread_t {
    cpu_state_t cpu_state;
    void* stack;
    void* stack_og;
    uint64_t tls[VTHREAD_TLS_ENTRY_COUNT] = {};
    uint64_t vtid;
    vthread_state_t vt_state;
};

static vthread_t* g_vthreads[VTHREAD_MAX_COUNT] {};
static uint64_t   g_vthread_count = 0;
static uint64_t   g_current_vthread_index = 0;

bool vthread_add(vthread_t* vthread);
bool vthread_create(vthread_t* vthread, void(*entry)());
cpu_state_t* vthread_schedule(cpu_state_t* cpu_state);
void vthread_yield();

tls_base_t* vthread_get_tls();

#endif // __VIRTUAL_THREAD_HPP__