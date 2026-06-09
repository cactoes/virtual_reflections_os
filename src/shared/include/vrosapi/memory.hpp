//==========================================
/// @file       memory.hpp
/// @brief      vros kernel memory api
//==========================================

#pragma once

#ifndef VROS_MEMORY_HPP
#define VROS_MEMORY_HPP

#include "common.hpp"

void* syscall_malloc(size_t size);
bool syscall_free(void* ptr);

#endif // VROMEMORY_HPP