//==========================================
/// @file       mutex.hpp
/// @brief      a mutex implementation for x86
///             with atomic exchange, w/o disabling interrupts
///             cpu inefficient
//==========================================

#pragma once

#ifndef __MUTEX_HPP__
#define __MUTEX_HPP__

struct mutex_t {
    volatile int locked;
};

void mutex_init(mutex_t* mutex);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);

/// @brief helper class for managing mutex lock / unlocks automatically
class mutex_lock_guard {
public:
    mutex_lock_guard(mutex_t* mutex) : _mutex(mutex) {
        if (_mutex)
            mutex_lock(_mutex);
    }

    ~mutex_lock_guard() {
        if (_mutex)
            mutex_unlock(_mutex);
    }

    // no wierd shit
    mutex_lock_guard(const mutex_lock_guard& other) = delete;
    mutex_lock_guard& operator=(const mutex_lock_guard& other) = delete;
    mutex_lock_guard(mutex_lock_guard&& other) = delete;
    mutex_lock_guard& operator=(mutex_lock_guard&& other) = delete;

private:
    mutex_t* _mutex;
};

#endif // __MUTEX_HPP__