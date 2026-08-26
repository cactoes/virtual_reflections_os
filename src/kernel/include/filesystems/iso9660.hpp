#pragma once

#ifndef __ISO9660_HPP__
#define __ISO9660_HPP__

#include "common.hpp"
#include "drivers/storage/block_device.hpp"
#include "std/array.hpp"
#include "std/pointer.hpp"
#include "filesystems/vfs.hpp"

struct iso9660_lbs_msb_32 {
    u32 le;
    u32 be;
} __packed;

struct iso9660_lbs_msb_16 {
    u16 le;
    u16 be;
} __packed;

enum class iso9660_volume_type_t : u8 {
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
    u8 version;
    
    u8 data[2041];
} __packed;

struct iso9660_volume_boot_record_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    u8 version;
    
    char boot_system_identifier_raw[32];
    char boot_identifier_raw[32];
    u8 boot_system_use[1977];
} __packed;

struct iso9660_dir_record_t {
    u8 length;
    u8 ext_attr_length;
    iso9660_lbs_msb_32 extent_lba;
    iso9660_lbs_msb_32 data_length;
    u8 recording_date[7];
    u8 file_flags;
    u8 file_unit_size;
    u8 interleave_gap_size;
    iso9660_lbs_msb_16 volume_sequence_number;
    u8 name_len;
    char name[];
} __packed;

struct iso9660_volume_primary_volume_descriptor_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    u8 version;
    
    u8 unused0; // should be 0
    char system_identifier_raw[32];
    char volume_identifier_raw[32];

    u8 unused1[8]; // should be 0
    iso9660_lbs_msb_32 volume_space_size;

    u8 unused2[32]; // should be 0
    iso9660_lbs_msb_16 volume_set_size;
    iso9660_lbs_msb_16 volume_sequence_number;
    iso9660_lbs_msb_16 logical_block_size;
    iso9660_lbs_msb_32 path_table_size;
    
    u32 location_path_table_lba_le; // LBA location of the path table. The path table pointed to contains only little-endian values.
    u32 location_path_table_optional_lba_le; // LBA location of the optional path table. The path table pointed to contains only little-endian values. Zero means that no optional path table exists. 

    u32 location_path_table_lba_be;
    u32 location_path_table_optional_lba_be;

    u8 directory_entry_root[34];

    char volume_set_identifier_raw[128];
    char publisher_identifier_raw[128];
    char data_preparer_identifier_raw[128];
    char application_identifier_raw[128];

    char copyright_file_identifier_raw[37];
    char abstract_file_identifier_raw[37];
    char bibliographic_file_identifier_raw[37];
    u8 creation_date[17];
    u8 modification_date[17];
    u8 expiration_date[17];
    u8 effective_date[17];
    u8 file_structure_version;
    
    u8 unused3; // should be 0
    char application_used[512];
    u8 reserved[653];
} __packed;

struct iso9660_volume_descriptor_set_terminator_t {
    iso9660_volume_type_t type;
    char identifier_raw[5];
    u8 version;
} __packed;

struct iso9660_susp_entry_t {
    char signature[2];
    u8 length;
    u8 version;
} __packed;

struct iso9660_node_t {
    u64 lba;
    u64 size;
    bool is_directory;
    char name[256];
};

struct iso9660_fsdata_t {
    block_device_t* block_device;
    u64 volume_size;
    iso9660_node_t root_node;
    iso9660_volume_primary_volume_descriptor_t pvd;
};

bool iso9660_init(block_device_t* device, iso9660_fsdata_t* fs_data);
bool iso9660_find_node(iso9660_fsdata_t* fs_data, const char* path, u64 size, u64 lba, iso9660_node_t* out_node);
bool iso9660_directory_exists(iso9660_fsdata_t* fs_data, const char* path);
bool iso9660_file_exists(iso9660_fsdata_t* fs_data, const char* path);
bool iso9660_read(iso9660_fsdata_t* fs_data, const char* path, u8** out_data, size_t* out_size);
bool iso9660_list_directory(iso9660_fsdata_t* fs_data, const char* path, std::dynamic_array<iso9660_node_t>* out_nodes);
const block_device_t* iso9660_get_block_device(iso9660_fsdata_t* fs_data);
const filesystem_interface_t* get_iso9660_filesystem_interface();
bool iso9660_validate(const u8* buffer, size_t size);

#endif // __ISO9660_HPP__