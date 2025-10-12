//==========================================
/// @file       system.hpp
/// @brief      stores generic system info
//==========================================

#pragma once

#ifndef __SYSTEM_INFO_HPP__
#define __SYSTEM_INFO_HPP__

#include "multiboot.hpp"
#include "string.hpp"

struct system_info_manager_t {
    size_t memory_size;

    string manufacturer;
    string product_name;
    string version;
    string serial_number;
};

void set_global_system_info_manager(system_info_manager_t* boot_info_manager);
system_info_manager_t* get_global_system_info_manager();

void system_info_parse_memory_size(system_info_manager_t* boot_info_manager, multiboot_t* multiboot_struct);
void system_info_parse_system_information(system_info_manager_t* boot_info_manager);

#endif // __SYSTEM_INFO_HPP__