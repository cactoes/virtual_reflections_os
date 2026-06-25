//==========================================
/// @file       process.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#define PROCESS_ID_INVALID  (process_id_t)-1
#define PROCESS_ID_KERNEL   0

#include "common.hpp"
#include "memory/heap.hpp"
#include "virtual_thread.hpp"
#include "arch/arch_selector.hpp"

typedef u64 process_id_t;

static process_id_t global_pid_counter = 1;

struct process_t {
    process_id_t pid;

    heap_t heap;
    vthread_handle_t main_thread;

    // i think this is correct for multiple cpu architectures
    void* page_table;

    // handles
    // idk[]

    u8* data;
    size_t data_size;
    void* start_address;

    void* program_data_v;

    bool is_kernel_process;
};

process_t* get_current_process();
void set_current_process(process_t* p_process);
bool create_process(process_t* process, const char* path);
bool process_setup_kernel_process(process_t* process, heap_t* heap);
void delete_process(process_t* process);

#endif // __PROCESS_HPP__