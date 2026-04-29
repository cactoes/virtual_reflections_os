#pragma once

#ifndef __BLOCK_DEVICE_HPP__
#define __BLOCK_DEVICE_HPP__

#include "common.hpp"

enum class block_device_type_t {
    UNKOWN = 0,
    IDE,
    AHCI
};

struct block_device_t {
    void* disk_device;
    block_device_type_t type;
    uint64_t start_lba;
    uint64_t end_lba;
    size_t block_size;
};

bool block_read_sized(block_device_t* device, uint64_t lba, uint8_t* buffer, size_t size);
bool block_read(block_device_t* device, uint64_t lba, uint8_t* buffer);

#endif // __BLOCK_DEVICE_HPP__