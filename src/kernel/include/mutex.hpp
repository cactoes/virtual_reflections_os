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

#endif // __MUTEX_HPP__