//==========================================
/// @file       vthread.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AMD64_VTHREAD_HPP__
#define __AMD64_VTHREAD_HPP__

#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

void amd64_vthread_store_context(struct vthread_t* target, void* stack);
void amd64_vthread_load_context(struct vthread_t* target);
bool amd64_vthread_init(struct vthread_t* thread, void* thread_entry);
bool amd64_vthread_init_main_thread(struct vthread_t* thread);
void amd64_vthread_cleanup(struct vthread_t* thread);

#endif

#endif // __AMD64_VTHREAD_HPP__