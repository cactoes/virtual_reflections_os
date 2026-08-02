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

enum class smbios_type_t : u8 {
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
    u8 anchor[4];
    u8 checksum;
    u8 length;
    u8 major_version;
    u8 minor_version;
    u16 max_structure_size;
    u8 entry_point_revision;
    u8 formatted_area[5];
    u8 intermediate_anchor[5];
    u8 intermediate_checksum;
    u16 table_length;
    u32 table_address;
    u16 structure_count;
    u8 bcd_revision;
} __packed;

struct smbios64_t {
    u8 anchor[5];
    u8 checksum;
    u8 length;
    u8 major_version;
    u8 minor_version;
    u8 docrev;
    u8 entry_point_revision;
    u8 reserved;
    u32 table_max_size;
    u64 table_address;
} __packed;

struct smbios_entry_header_t {
    smbios_type_t type;
    u8 length;
    u16 handle;
};

struct smbios_entry_system_information_t {
    smbios_type_t type;
    u8 length;
    u16 handle;
    
    u8 manufacturer_str_index;
    u8 product_name_str_index;
    u8 version_str_index;
    u8 serial_number_str_index;

    u8 uuid[16];
    u8 wakeup_type;
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
void smbios_iterate(u64 table_address, void* extra, bool(*callback)(smbios_entry_header_t* entry, void* extra));

/// @brief                      find a struct entry in memory by signature
/// @param[in] signature        pointer to the signature bytes
/// @param[in] signature_size   size of the signature in bytes
/// @return                     pointer to the found entry or nullptr if not found
void* smbios_find_struct_entry(const char* signature, size_t signature_size);

#endif // __SMBIOS_HPP__