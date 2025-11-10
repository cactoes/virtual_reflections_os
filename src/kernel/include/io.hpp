//==========================================
/// @file       io.hpp
/// @brief      in / out stuff for terminals etc
//==========================================s

#pragma once

#ifndef __IO_HPP__
#define __IO_HPP__

#define IO_PRINT_BUFFER_SIZE 512

#include "common.hpp"

enum class io_stream_t {
    // negative == context specific
    STD_OUT = -1,
    STD_ERR = -2,
    STD_WRN = -3,

    // port debug stream
    STD_DBG = 0,

    // other
    // ...
};

enum class io_flag {
    KPRINT_BYPASS_VFS,
    __SIZE
};

bool write_stream(io_stream_t stream, const char* str);
void printf(const char* str, ...);
void kprintf(const char* str, ...);
void io_flag_set(io_flag flag, bool state);
bool io_flag_get(io_flag flag);

#endif // __IO_HPP__