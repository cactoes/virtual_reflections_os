//==========================================
/// @file       disk_manager.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DISK_MANAGER_HPP__
#define __DISK_MANAGER_HPP__

#include "common.hpp"
#include "std/array.hpp"

struct disk_interface_t {
    bool(*read)(void* disk_data, u64 lba, u8* buffer, u64 size);
    void(*write)(void* disk_data, u64 lba, const u8* buffer, u64 size);
    u64(*get_sector_size)(void* disk_data);
    u64(*get_capacity)(void* disk_data);

    const char*(*get_model)(void* disk_data);
    const char*(*get_serial)(void* disk_data);
    const char*(*get_firmware)(void* disk_data);
};

struct disk_t {
    char name[32];
    const disk_interface_t* interface;
    void* disk_data;
};

struct disk_manager_t {
    std::dynamic_array<disk_t> disks;
};

disk_manager_t* get_global_disk_manager();
void set_global_disk_manager(disk_manager_t* disk_manager);

bool disk_manager_register(disk_manager_t* disk_manager, const char* name, const disk_interface_t* interface, void* disk_data);

#endif // __DISK_MANAGER_HPP__