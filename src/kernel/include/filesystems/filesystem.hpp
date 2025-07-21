//==========================================
/// @file       filesystem.hpp
/// @brief      generic way to interact with the filesystem
//==========================================

#pragma once

#ifndef __FILESYSTEMS_FILESYSTEM_HPP__
#define __FILESYSTEMS_FILESYSTEM_HPP__

#include "common.hpp"
#include "drivers/storage/storage.hpp"

struct filesystem_api_t {
    int(*read)(filesystem_api_t*, const char* p_path, void** p_data, size_t* p_size);
    int(*write)(filesystem_api_t*, const char* p_path, void* p_data, size_t* p_size);
};

#endif // __FILESYSTEMS_FILESYSTEM_HPP__