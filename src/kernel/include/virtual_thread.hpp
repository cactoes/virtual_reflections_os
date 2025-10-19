//==========================================
/// @file       virtual_thread.hpp
/// @brief      very basic implementation of multitasking
///  TODO       vthread cleanup
///             wrapper start function
///             better thread hander
//==========================================

#pragma once

#ifndef __VIRTUAL_THREAD_HPP__
#define __VIRTUAL_THREAD_HPP__

#define VTHREAD_STACK_SIZE          1000000     // 1M (randomly chosen)
#define VTHREAD_STACK_DEADZONE      64000       // 64K (randomly chosen)
#define VTHREAD_TLS_ENTRY_COUNT     64
#define VTHREAD_MAIN_THREAD_HANDLE  (vthread_handle_t)0

#define VTHREAD_HANDLE_INVALID (vthread_handle_t)-1

#include "common.hpp"

typedef uint64_t vthread_handle_t;
typedef int(*thread_entry_t)();

struct tls_base_t {
    vthread_handle_t handle;
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
    void* stack_bottom;
    uint64_t tls[VTHREAD_TLS_ENTRY_COUNT] = {};
    vthread_handle_t handle;
    vthread_state_t vt_state;
    int exit_code;
    uint64_t sleep_until_ms;
    void* pml4;
};

/// @brief      start the main virtual thread & perform initial setup
/// @return     handle to the newly created main virtual thread
vthread_handle_t vthread_start_and_setup_main();

/// @brief                      create a new vthread
/// @param[in] p_thread_entry   entry point function for the new thread
/// @param[in] pml4             pointer to the pml4 for the thread address space
/// @return                     handle to the created vthread
vthread_handle_t vthread_create(thread_entry_t p_thread_entry, void* pml4);

/// @brief                      handles a vthread interrupt & updates cpu state
/// @param[in] p_cpu_state      pointer to the current cpu state
/// @return                     pointer to the updated cpu state
cpu_state_t* vthread_handle_interrupt(cpu_state_t* p_cpu_state);

/// @brief                      schedules the next virtual thread & switches context
/// @param[in] p_cpu_state      pointer to the current cpu state
/// @return                     pointer to the cpu state of the next scheduled thread
cpu_state_t* vthread_schedule(cpu_state_t* p_cpu_state);

/// @brief      yield execution of the current virtual thread to allow other threads to run
void vthread_yield();

/// @brief      get the tls base for the current virtual thread
/// @return     pointer to the tls base of the current thread
tls_base_t* vthread_get_tls();

/// @brief                  puts the current thread to sleep for a given time in ms
/// @param time_ms          number of milliseconds to sleep
/// @remarks                main thread (handle 0) is not allowed to sleep
void vthread_sleep(uint64_t time_ms);

/// @brief                  waits until the thread with the given handle is closed & removed
/// @param handle           handle of the thread to wait for
/// @return                 always returns 0 after the thread is closed
int vthread_wait_for_close(vthread_handle_t handle);

/// @brief      get the current number of virtual threads
/// @return     number of threads currently managed by the scheduler
size_t vthread_get_count();

/// @brief          closes a thread forcefully
/// @param handle   handle to the thread
void vthread_terminate(vthread_handle_t handle);

#endif // __VIRTUAL_THREAD_HPP__