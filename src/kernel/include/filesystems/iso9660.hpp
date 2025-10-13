//==========================================
/// @file       iso9660.hpp
/// @brief      iso9660 file system implementation
//==========================================

#pragma once

#ifndef __FILESYSTEMS_ISO9660_HPP__
#define __FILESYSTEMS_ISO9660_HPP__

#define SECTOR_SIZE 2048


#include "common.hpp"
#include "filesystems/filesystem.hpp"
#include "drivers/storage/storage.hpp"
#include "std/array.hpp"
#include "string.hpp"
#include "std/pointer.hpp"

struct iso9660_lbs_msb_32 {
    uint32_t le;
    uint32_t be;
} PACKED;

struct iso9660_lbs_msb_16 {
    uint16_t le;
    uint16_t be;
} PACKED;

enum class iso9660_volume_type_t : uint8_t {
    BOOT_RECORD = 0,
    PRIMARY_VOLUME_DESCRIPTOR,
    SUPPLEMENTARY_VOLUME_DESCRIPTOR,
    VOLUME_PARTITION_DESCRIPTOR,
    // Reserved 4-254
    VOLUME_DESCRIPTOR_SET_TERMINATOR = 255
};

struct iso9660_volume_descriptor_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
    
    uint8_t data[2041];
} PACKED;

struct iso9660_volume_boot_record_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
    
    char boot_system_identifier_raw[32];
    char boot_identifier_raw[32];
    uint8_t boot_system_use[1977];
} PACKED;

struct iso9660_dir_record_t {
    uint8_t length;
    uint8_t ext_attr_length;
    iso9660_lbs_msb_32 extent_lba;
    iso9660_lbs_msb_32 data_length;
    uint8_t recording_date[7];
    uint8_t file_flags;
    uint8_t file_unit_size;
    uint8_t interleave_gap_size;
    iso9660_lbs_msb_16 volume_sequence_number;
    uint8_t name_len;
    char name[];
} PACKED;

struct iso9660_volume_primary_volume_descriptor_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
    
    uint8_t unused0; // should be 0
    char system_identifier_raw[32];
    char volume_identifier_raw[32];

    uint8_t unused1[8]; // should be 0
    iso9660_lbs_msb_32 volume_space_size;

    uint8_t unused2[32]; // should be 0
    iso9660_lbs_msb_16 volume_set_size;
    iso9660_lbs_msb_16 volume_sequence_number;
    iso9660_lbs_msb_16 logical_block_size;
    iso9660_lbs_msb_32 path_table_size;
    
    uint32_t location_path_table_lba_le; // LBA location of the path table. The path table pointed to contains only little-endian values.
    uint32_t location_path_table_optional_lba_le; // LBA location of the optional path table. The path table pointed to contains only little-endian values. Zero means that no optional path table exists. 

    uint32_t location_path_table_lba_be;
    uint32_t location_path_table_optional_lba_be;

    uint8_t directory_entry_root[34];

    char volume_set_identifier_raw[128];
    char publisher_identifier_raw[128];
    char data_preparer_identifier_raw[128];
    char application_identifier_raw[128];

    char copyright_file_identifier_raw[37];
    char abstract_file_identifier_raw[37];
    char bibliographic_file_identifier_raw[37];
    uint8_t creation_date[17];
    uint8_t modification_date[17];
    uint8_t expiration_date[17];
    uint8_t effective_date[17];
    uint8_t file_structure_version;
    
    uint8_t unused3; // should be 0
    char application_used[512];
    uint8_t reserved[653];
} PACKED;

struct iso9660_volume_descriptor_set_terminator_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    uint8_t version;
} PACKED;

struct susp_entry_t {
    char signature[2];
    uint8_t length;
    uint8_t version;
} PACKED;

struct iso9660_node_data_t {
    uint64_t lba;
    uint32_t size;
    bool is_directory;
};

struct iso9660_data_t {
    iso9660_volume_primary_volume_descriptor_t pvd;
    uint32_t block_size;
    uint32_t volume_size;
    iso9660_node_data_t root;
};

class iso9660_filesystem_interface_t : public filesystem_interface_t {
public:
    iso9660_filesystem_interface_t(std::unique_ptr<storage_driver_interface_t> storage_interface, const iso9660_data_t& data);

    bool read(const char* path, void** data, size_t* size) override;
    bool write(const char* path, void* data, size_t* size) override;
    bool enumerate_directory(const char* path, std::dynamic_array<filesystem_node_t>* out_array) override;
    const storage_driver_interface_t* get_storage_interface() const override;

private:
    iso9660_data_t data;
    std::unique_ptr<storage_driver_interface_t> storage_interface;
};

int iso9660_init(storage_driver_interface_t* storage_interface, iso9660_data_t* fs_data);

#endif // __FILESYSTEMS_ISO9660_HPP__