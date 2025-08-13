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

#define VTHREAD_STACK_SIZE          8192 // 8kb
// #define VTHREAD_MAX_COUNT           256

#define VTHREAD_TLS_ENTRY_COUNT     64

#include "common.hpp"

typedef uint64_t vthread_handle_t;
typedef int(*thread_entry_t)();

struct tls_base_t {
    vthread_handle_t handle;
    uint64_t is_yielded;
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
    vthread_handle_t handle;
    vthread_state_t vt_state;
    int exit_code;
};

bool vthread_start_and_setup_main(vthread_t* p_vthread);
bool vthread_add(vthread_t* p_vthread);
bool vthread_create(vthread_t* p_vthread, thread_entry_t p_thread_entry);
cpu_state_t* vthread_interrupt_handler(cpu_state_t* p_cpu_state);
cpu_state_t* vthread_schedule(cpu_state_t* p_cpu_state);
void vthread_yield();
tls_base_t* vthread_get_tls();

#endif // __VIRTUAL_THREAD_HPP__