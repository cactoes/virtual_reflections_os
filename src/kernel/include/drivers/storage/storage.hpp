//==========================================
/// @file       storage.hpp
/// @brief      generic way to talk to storage drivers
//==========================================

#pragma once

#ifndef __DRIVERS_STORAGE_STORAGE_HPP__
#define __DRIVERS_STORAGE_STORAGE_HPP__

#include "common.hpp"

// struct storage_driver_api_t {
//     int(*read)(storage_driver_api_t*, uint32_t lba, uint8_t* p_buffer, size_t size);
//     int(*write)(storage_driver_api_t*, uint32_t lba, uint8_t* p_buffer, size_t size);
// };

struct storage_driver_interface_t {
    virtual ~storage_driver_interface_t() = default;

    virtual bool read(uint32_t lba, uint8_t* buffer, size_t size) = 0;
    virtual bool write(uint32_t lba, uint8_t* buffer, size_t size) = 0;
};

#endif // __DRIVERS_STORAGE_STORAGE_HPP__