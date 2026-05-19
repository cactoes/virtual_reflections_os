//==========================================
/// @file       process.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include "common.hpp"
#include "memory/heap.hpp"

typedef u64 vthread_handle_t;

struct process_t {
    heap_t heap;
    void* page_table;
    vthread_handle_t main_thread;

    u8* data;
    size_t data_size;
    void* start_address;
};

process_t* get_current_process();
void set_current_process(process_t* p_process);
bool create_process(process_t* process, const char* path);

#endif // __PROCESS_HPP__