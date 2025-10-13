//==========================================
/// @file       storage.hpp
/// @brief      generic way to talk to storage drivers
//==========================================

#pragma once

#ifndef __DRIVERS_STORAGE_STORAGE_HPP__
#define __DRIVERS_STORAGE_STORAGE_HPP__

#include "common.hpp"
#include "utils/pointer.hpp"
#include "string.hpp"

struct storage_info_t {
    string model;
    string serial;
    string firmare;

    size_t capacity;
};

struct storage_driver_interface_t {
    virtual ~storage_driver_interface_t() = default;

    virtual bool read(uint32_t lba, uint8_t* buffer, size_t size) = 0;
    virtual bool write(uint32_t lba, uint8_t* buffer, size_t size) = 0;
    virtual size_t get_block_size() = 0;
    virtual void set_root_lba(uint64_t lba) = 0;
    virtual uint64_t get_root_lba() = 0;
    virtual storage_info_t get_storage_info() const = 0;
};

bool mount_disk(ptr::unique<storage_driver_interface_t> interface, const char* path);

#endif // __DRIVERS_STORAGE_STORAGE_HPP__