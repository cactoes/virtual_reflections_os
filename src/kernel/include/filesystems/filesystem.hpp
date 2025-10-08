//==========================================
/// @file       filesystem.hpp
/// @brief      generic way to interact with the filesystem
//==========================================

#pragma once

#ifndef __FILESYSTEMS_FILESYSTEM_HPP__
#define __FILESYSTEMS_FILESYSTEM_HPP__

#include "common.hpp"
#include "drivers/storage/storage.hpp"
#include "string.hpp"
#include "utils/vector.hpp"

// very basic node for simple info
struct filesystem_node_t {
    string name;
    bool is_directory;
    size_t filesize;
};


struct filesystem_api_t {
    int(*read)(filesystem_api_t*, const char* p_path, void** p_data, size_t* p_size);
    int(*write)(filesystem_api_t*, const char* p_path, void* p_data, size_t* p_size);
    bool(*enumerate_directory)(filesystem_api_t*, const char* path, dynamic_array<filesystem_node_t>* out_array);
};

#endif // __FILESYSTEMS_FILESYSTEM_HPP__