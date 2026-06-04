//==========================================
/// @file       virtual_thread.hpp
/// @brief      very basic implementation of multitasking
//==========================================

#pragma once

#ifndef __VIRTUAL_THREAD_HPP__
#define __VIRTUAL_THREAD_HPP__

#define VTHREAD_STACK_SIZE          2000000     // 2M (randomly chosen)
#define VTHREAD_STACK_DEADZONE      64000       // 64K (randomly chosen)
#define VTHREAD_MAIN_THREAD_HANDLE  (vthread_handle_t)0
#define VTHREAD_MAX_NAME_SIZE       63          // 63 actual chars, + a null terminator which makes 64

#define VTHREAD_HANDLE_INVALID (vthread_handle_t)-1

#define __thread_tls vthread_get_tls()

#include "common.hpp"
#include "filesystems/vfs.hpp"
#include "std/pointer.hpp"
#include "process.hpp"
#include "arch/arch_selector.hpp"

typedef u64 vthread_handle_t;
typedef int(*thread_entry_t)();

struct critical_section_t {
    bool is_locked;
};

struct thread_local_storage_t {
    vthread_handle_t handle;
    // file_descriptor_t out_streams[3] { FILE_DESCRIPTOR_INVALID, FILE_DESCRIPTOR_INVALID, FILE_DESCRIPTOR_INVALID };
    u64 saved_irq_flags;
    int irq_disable_depth;
};

enum class vthread_state_t {
    UNKNOWN = 0,
    STARTING,
    RUNNING,
    SLEEPING,
    STOPPING
};

struct vthread_t {
    // handle to thread
    vthread_handle_t handle;

    // handle to parent process
    process_t* parent;
    
    // state of thread
    vthread_state_t vt_state;
    
    // tls data
    thread_local_storage_t tls;

    // stack stuff
    void* stack_top;
    void* stack_bottom;
    void* stack_bottom_kernel;
    void* kstack;

    // internal thread stuff
    u64 sleep_until_ms;
    int exit_code;
    bool is_critical;
    char name[VTHREAD_MAX_NAME_SIZE + 1];

#if CPU_ARCHITECTURE == ARCH_AMD64
    // memory map
    void* page_table;

    // 64-byte aligned fpu area
    u8* fpu_state;
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif
};

/// @brief                  start the main virtual thread & perform initial setup
/// @return                 handle to the newly created main virtual thread
vthread_handle_t vthread_start_and_setup_main();

/// @brief                      create a new vthread
/// @param[in] p_thread_entry   entry point function for the new thread
/// @param[in] pml4             pointer to the pml4 for the thread address space
/// @return                     handle to the created vthread
vthread_handle_t vthread_create_local(thread_entry_t p_thread_entry, const char name[VTHREAD_MAX_NAME_SIZE] = nullptr);

/// @brief                      handles a vthread interrupt & updates cpu state
/// @param[in] stack            pointer to last stack
/// @param[in] UNUSED           pointer to context data
/// @return                     pointer to (new) stack
void* vthread_handle_interrupt(void* stack, void*);

/// @brief                      schedules the next virtual thread & switches context
/// @param[in] stack            pointer to last stack
/// @return                     pointer to a (new) stack
void* vthread_schedule(void* stack);

/// @brief      yield execution of the current virtual thread to allow other threads to run
void vthread_yield();

/// @brief      get the tls base for the current virtual thread
/// @return     pointer to the tls base of the current thread
thread_local_storage_t* vthread_get_tls();

/// @brief                  puts the current thread to sleep for a given time in ms
/// @param time_ms          number of milliseconds to sleep
/// @remarks                main thread (handle 0) is not allowed to sleep
void vthread_sleep(u64 time_ms);

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

/// @brief          closes current thread forcefully
void vthread_terminate();

bool vthread_is_closed(vthread_handle_t handle);

vthread_t* vthread_get(vthread_handle_t handle);

bool vthread_set_critical(vthread_handle_t handle, bool state = true);

vthread_state_t vthread_get_state();

// critical_section_t enter_critical_section(bool wait_for_lock = true, bool can_fail = false);
// bool leave_critical_section(critical_section_t* section);

bool vthread_add(std::unique_ptr<vthread_t> p_vthread);

vthread_handle_t vhtread_next_handle();

#endif // __VIRTUAL_THREAD_HPP__