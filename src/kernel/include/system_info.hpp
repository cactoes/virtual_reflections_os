//==========================================
/// @file       system.hpp
/// @brief      stores generic system info
//==========================================

#pragma once

#ifndef __SYSTEM_INFO_HPP__
#define __SYSTEM_INFO_HPP__

#include "multiboot.hpp"
#include "std/string.hpp"

struct system_info_manager_t {
    size_t memory_size;

    std::string manufacturer;
    std::string product_name;
    std::string version;
    std::string serial_number;

    std::string cpu_name;
};

/// @brief                          set the global system info manager
/// @param[in] system_info_manager  pointer to the system info manager
void set_global_system_info_manager(system_info_manager_t* system_info_manager);

/// @brief              get the global system info manager
/// @return             pointer to the current global system info manager
system_info_manager_t* get_global_system_info_manager();

/// @brief                              parse memory size from the multiboot memory map
/// @param[inout] system_info_manager   system info manager to store memory info
/// @param[in] multiboot_struct         pointer to the multiboot structure
void system_info_parse_memory_size(system_info_manager_t* system_info_manager, multiboot2_info_t* multiboot_struct);

/// @brief                              parse system information from smbios tables
/// @param[inout] system_info_manager   system info manager to store manufacturer, product, version & serial
void system_info_parse_system_information(system_info_manager_t* system_info_manager);

/// @brief                              gets the cpu name of the system
/// @param[inout] system_info_manager   system info manager to store the name in
void system_info_get_cpu_name(system_info_manager_t* system_info_manager);

#endif // __SYSTEM_INFO_HPP__