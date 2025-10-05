//==========================================
/// @file       smbios.hpp
/// @brief      system management bios
//==========================================

#pragma once

#ifndef __SMBIOS_HPP__
#define __SMBIOS_HPP__

#include "common.hpp"

enum class smbios_type_t : uint8_t {
    BIOS_INFORMATION = 0,
    SYSTEM_INFORMATION = 1,
    MAINBOARD_INFORMATION = 2,
    ENCLOSURE_CHASIS_INFORMATION = 3,
    PROCESSOR_INFROMATION = 4,
    CACHE_INFORMATION = 7,
    SYSTEM_SLOTS_INFORMATION = 9,
    PHYSICAL_MEMORY_ARRAY = 16,
    MEMORY_DEVICE_INFORMATION = 17,
    MEMORY_ARRAY_MAPPED_ADDRESS = 19,
    MEMORY_DEVICE_MAPPED_ADDRESS = 20,
    SYSTEM_BOOT_INFORMATION = 32,
    END_OF_TABLE = 127
};

struct smbios_t {
    uint8_t anchor[4];
    uint8_t checksum;
    uint8_t length;
    uint8_t major_version;
    uint8_t minor_version;
    uint16_t max_structure_size;
    uint8_t entry_point_revision;
    uint8_t formatted_area[5];
    uint8_t intermediate_anchor[5];
    uint8_t intermediate_checksum;
    uint16_t table_length;
    uint32_t table_address;
    uint16_t structure_count;
    uint8_t bcd_revision;
} PACKED;

struct smbios_entry_header_t {
    smbios_type_t type;
    uint8_t length;
    uint16_t handle;
};

struct smbios_entry_system_information_t {
    smbios_type_t type;
    uint8_t length;
    uint16_t handle;
};

size_t smbios_entry_length(const smbios_entry_header_t* entry);
void smbios_test();

#endif // __SMBIOS_HPP__