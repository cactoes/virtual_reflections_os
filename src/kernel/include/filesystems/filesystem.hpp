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

// struct filesystem_api_t {
//     int(*read)(filesystem_api_t*, const char* p_path, void** p_data, size_t* p_size);
//     int(*write)(filesystem_api_t*, const char* p_path, void* p_data, size_t* p_size);
//     bool(*enumerate_directory)(filesystem_api_t*, const char* path, dynamic_array<filesystem_node_t>* out_array);
// };

struct filesystem_interface_t {
    virtual ~filesystem_interface_t() = default;

    virtual bool read(const char* path, void** data, size_t* size) = 0;
    virtual bool write(const char* path, void* data, size_t* size) = 0;
    virtual bool enumerate_directory(const char* path, dynamic_array<filesystem_node_t>* out_array) = 0;
};

#endif // __FILESYSTEMS_FILESYSTEM_HPP__