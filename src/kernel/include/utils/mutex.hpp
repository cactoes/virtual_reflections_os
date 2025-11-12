//==========================================
/// @file       mutex.hpp
/// @brief      a mutex implementation
///             with atomic exchange, w/o disabling interrupts
///             cpu inefficient
//==========================================

#pragma once

#ifndef __UTILS_MUTEX_HPP__
#define __UTILS_MUTEX_HPP__

#include "common.hpp"

typedef uint64_t vthread_handle_t;

struct mutex_t {
    volatile int locked;
    uint64_t saved_flags;

    // for crash handler cleanup
    vthread_handle_t handle;
};

void mutex_init(mutex_t* p_mutex);
void mutex_lock(mutex_t* p_mutex);
void mutex_unlock(mutex_t* p_mutex);
void mutex_clear_all_thread_references_and_release(vthread_handle_t handle);

/// @brief helper class for managing mutex lock / unlocks automatically
class mutex_lock_guard {
public:
    mutex_lock_guard(mutex_t* p_mutex) : m_mutex(p_mutex) {
        if (m_mutex)
            mutex_lock(m_mutex);
    }

    ~mutex_lock_guard() {
        if (m_mutex)
            mutex_unlock(m_mutex);
    }

    // no wierd shit
    mutex_lock_guard(const mutex_lock_guard& other) = delete;
    mutex_lock_guard& operator=(const mutex_lock_guard& other) = delete;
    mutex_lock_guard(mutex_lock_guard&& other) = delete;
    mutex_lock_guard& operator=(mutex_lock_guard&& other) = delete;

private:
    mutex_t* m_mutex;
};

#endif // __UTILS_MUTEX_HPP__