//==========================================
/// @file       virtual_thread.hpp
/// @brief      very basic implementation of multitasking
//==========================================

#pragma once

#ifndef __VIRTUAL_THREAD_HPP__
#define __VIRTUAL_THREAD_HPP__

#define VTHREAD_STACK_SIZE 8192         // 8KB
#define VTHREAD_MAX_COUNT  256

#include "cpu.hpp"
#include "memory.hpp"

struct vthread_t {
    cpu_state_t cpu_state;
    void* stack;

    // TODO @since 14/04/2025 -- 13:58
    // extra info like: thread sleep
};

static vthread_t* g_vthreads[VTHREAD_MAX_COUNT] {};
static uint64_t   g_vthread_count = 0;
static uint64_t   g_current_vthread_index = 0;

bool vthread_add(vthread_t* vthread);
bool vthread_create(vthread_t* vthread, void(*entry)());
cpu_state_t* vthread_schedule(cpu_state_t* cpu_state);

#endif // __VIRTUAL_THREAD_HPP__