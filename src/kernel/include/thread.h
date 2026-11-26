//==========================================
/// @file       thread.h
/// @brief      
//==========================================

#pragma once

#ifndef THREAD_H
#define THREAD_H

#include "libtype.h"

typedef u64 vthread_handle_t;

typedef enum __vthread_state {
    UNKNOWN = 0,
    RUNNING,
    READY,
    WAITING,
    BLOCKED,
    SLEEPING,
    ZOMBIE
} vthread_state_t;

typedef struct __vthread {
    vthread_handle_t handle;
    vthread_state_t state;
    struct __tvhread* parent;
} thread_t;

#endif // THREAD_H