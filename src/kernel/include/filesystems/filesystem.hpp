//==========================================
/// @file       filesystem.hpp
/// @brief      generic way to interact with the filesystem
//==========================================

#pragma once

#ifndef __FILESYSTEMS_FILESYSTEM_HPP__
#define __FILESYSTEMS_FILESYSTEM_HPP__

#define MBR_PARTITION_OFFSET 0x1BE

#include "common.hpp"
#include "drivers/storage/storage.hpp"
#include "std/string.hpp"
#include "std/array.hpp"

struct mbr_partition_entry_t {
    uint8_t boot_flag;
    uint8_t chs_begin[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t sector_count;
} PACKED;

enum class filesystem_type_t {
    UNKNOWN,
    FAT32,
    ISO9660
};

// very basic node for simple info
struct filesystem_node_t {
    std::string name;
    bool is_directory;
    size_t filesize;
};

struct filesystem_interface_t {
    virtual ~filesystem_interface_t() = default;

    virtual bool read(const char* path, void** data, size_t* size) = 0;
    virtual bool write(const char* path, void* data, size_t* size) = 0;
    virtual bool enumerate_directory(const char* path, std::dynamic_array<filesystem_node_t>* out_array) = 0;
    virtual const storage_driver_interface_t* get_storage_interface() const = 0;
};

filesystem_type_t filesystem_identify(storage_driver_interface_t* storage_interface);

#endif // __FILESYSTEMS_FILESYSTEM_HPP__