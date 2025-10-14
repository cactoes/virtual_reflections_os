//==========================================
/// @file       smbios.hpp
/// @brief      system management bios
//==========================================

#pragma once

#ifndef __SMBIOS_HPP__
#define __SMBIOS_HPP__

#define SMBIOS_SIGNATUE     "_SM_"
#define SMBIOS64_SIGNATUE   "_SM3_"

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

struct smbios64_t {
    uint8_t anchor[5];
    uint8_t checksum;
    uint8_t length;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t docrev;
    uint8_t entry_point_revision;
    uint8_t reserved;
    uint32_t table_max_size;
    uint64_t table_address;
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
    
    uint8_t manufacturer_str_index;
    uint8_t product_name_str_index;
    uint8_t version_str_index;
    uint8_t serial_number_str_index;

    uint8_t uuid[16];
    uint8_t wakeup_type;
};

/// @brief              get the length of a smbios entry
/// @param[in] entry    pointer to the smbios entry header
/// @return             length of the entry in bytes
size_t smbios_entry_length(const smbios_entry_header_t* entry);

/// @brief              get string at index from a smbios entry
/// @param[in] entry    pointer to the smbios entry header
/// @param[in] i        string index
/// @return             pointer to the string or nullptr if not found
const char* smbios_get_string_at_index(const smbios_entry_header_t* entry, size_t i);

/// @brief                      iterate over all smbios entries and call callback for each
/// @param[in] table_address    address of the smbios table
/// @param[inout] extra         user data passed to callback
/// @param[in] callback         function called for each entry, return true to continue
void smbios_iterate(uint64_t table_address, void* extra, bool(*callback)(smbios_entry_header_t* entry, void* extra));

/// @brief                      find a struct entry in memory by signature
/// @param[in] signature        pointer to the signature bytes
/// @param[in] signature_size   size of the signature in bytes
/// @return                     pointer to the found entry or nullptr if not found
void* smbios_find_struct_entry(const char* signature, size_t signature_size);

#endif // __SMBIOS_HPP__