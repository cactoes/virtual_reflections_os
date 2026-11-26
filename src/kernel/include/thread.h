//==========================================
/// @file       thread.h
/// @brief      
//==========================================

#pragma once

#ifndef THREAD_H
#define THREAD_H

#define VTHREAD_STACK_SIZE 0x1000

#include "libtype.h"
#include "process.h"

typedef u64 vthread_handle_t;

typedef enum __vthread_state {
    VTHREAD_UNKNOWN = 0,
    VTHREAD_READY,
    VTHREAD_RUNNING,
    VTHREAD_BLOCKED,
    VTHREAD_SLEEPING,
    VTHREAD_ZOMBIE
} vthread_state_t;

typedef struct __vthread {
    vthread_handle_t handle;
    vthread_state_t state;
    
    process_t* process;

    void* stack_base;
    void* stack_top;
    void* context;
    void* entry;

    struct __vthread* next;
} vthread_t;

typedef struct __vthread_queue {
    vthread_t* head;
    vthread_t* tail;
} vthread_queue_t;

bool vthread_scheduler_init();

bool vthread_enqueue(vthread_t* thread);

void* vthread_schedule(void* frame);
bool vthread_scheduler_init_local(vthread_t* initial_thread, vthread_t* idle_thread);
bool vthread_create(process_t* process, void* entry, vthread_t* vthread);

#endif // THREAD_H