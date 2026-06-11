//==========================================
/// @file       file.hpp
/// @brief      
//==========================================

#pragma once

#ifndef VROS_FILE_HPP
#define VROS_FILE_HPP

#include "common.hpp"

u64 syscall_open_file(const char* path);
bool syscall_read_file(u64 handle, u8** data, u64* size);

#endif // VROS_FILE_HPP