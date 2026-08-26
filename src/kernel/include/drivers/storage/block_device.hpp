#pragma once

#ifndef __BLOCK_DEVICE_HPP__
#define __BLOCK_DEVICE_HPP__

#include "common.hpp"
#include "storage/disk_manager.hpp"

// enum class block_device_type_t {
//     UNKOWN = 0,
//     IDE,
//     AHCI
// };

// struct block_device_t {
//     void* disk_device;
//     block_device_type_t type;
//     u64 start_lba;
//     u64 end_lba;
//     size_t block_size;
// };

// bool block_read_sized(block_device_t* device, u64 lba, u8* buffer, size_t size);
// bool block_read(block_device_t* device, u64 lba, u8* buffer);
// const char* block_device_get_model(block_device_t* device);
// const char* block_device_get_serial(block_device_t* device);
// const char* block_device_get_firmware(block_device_t* device);
// u64 block_device_get_drive_capacity(block_device_t* device);

struct block_device_t {
    char name[32];
    const disk_interface_t* interface;
    void* disk_data;
    u64 start_lba;
    u64 end_lba;
    u64 block_size;
};

bool block_device_read(block_device_t* block_device, u64 lba, u8* buffer, u64 size);
bool block_device_read_sized(block_device_t* device, u64 lba, u8* buffer, u64 size);

#endif // __BLOCK_DEVICE_HPP__