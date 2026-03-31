//==========================================
/// @file       fat32.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __FAT32_HPP__
#define __FAT32_HPP__

#define FAT32_DIRATTR_READ_ONLY     (1 << 0)
#define FAT32_DIRATTR_HIDDEN        (1 << 1)
#define FAT32_DIRATTR_SYSTEM        (1 << 2)
#define FAT32_DIRATTR_VOLUME_ID     (1 << 3)
#define FAT32_DIRATTR_DIRECTORY     (1 << 4)
#define FAT32_DIRATTR_ARCHIVE       (1 << 5)
#define FAT32_DIRATTR_LFN           (FAT32_DIRATTR_READ_ONLY | FAT32_DIRATTR_HIDDEN | FAT32_DIRATTR_SYSTEM | FAT32_DIRATTR_VOLUME_ID)

#define FAT32_EOC32                 0x0FFFFFF8u
#define FAT32_SECTOR_SIZE           512

#define FAT32_DATE_YEAR(d)   (uint16_t)(1980 + ((d >> 9) & 0x7F))
#define FAT32_DATE_MONTH(d)  (uint8_t)((d >> 5) & 0x0F)
#define FAT32_DATE_DAY(d)    (uint8_t)(d & 0x1F)

#define FAT32_TIME_HOUR(t)   (uint8_t)((t >> 11) & 0x1F)
#define FAT32_TIME_MIN(t)    (uint8_t)((t >> 5)  & 0x3F)
#define FAT32_TIME_SEC(t)    (uint8_t)((t & 0x1F) * 2)

#include "common.hpp"
#include "storage/block_device.hpp"
#include "std/array.hpp"
#include "std/pointer.hpp"

typedef uint16_t fat32_date_t;
typedef uint16_t fat32_time_t;

struct fat32_bpb_t {
    char jmp[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_large;
} PACKED;

struct fat32_bpb_extended_t {
    fat32_bpb_t bpb;

    uint32_t fat_size_32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[420];
    uint32_t boot_parition_signature;
} PACKED;

struct fat32_dir_entry_t {
    char name[11];
    uint8_t attributes;
    uint8_t nt_reserved;
    uint8_t creation_time_tenths;
    fat32_time_t creation_time;
    fat32_date_t creation_date;
    fat32_date_t last_access_date;
    uint16_t first_cluster_high;
    fat32_time_t write_time;
    fat32_date_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} PACKED;

struct fat32_lfn_entry_t {
    uint8_t order;
    uint16_t name1[5];
    uint8_t attr;
    uint8_t type;
    uint8_t checksum;
    uint16_t name2[6];
    uint16_t zero;
    uint16_t name3[2];
} PACKED;

struct fat32_node_t {
    uint32_t first_cluster;
    uint64_t size;
    bool is_directory;
    char name[256];
};

struct fat32_fsdata_t {
    std::unique_ptr<block_device_t> block_device;
    uint64_t volume_size;
    fat32_node_t root_node;

    struct {
        uint16_t bytes_per_sector;
        uint8_t  sectors_per_cluster;
        uint32_t sectors_per_fat;
        uint32_t first_fat_sector;
        uint32_t first_data_sector;
        uint32_t root_cluster;
    } layout;
};

bool fat32_validate(uint8_t* buffer, size_t size);

bool fat32_init(std::unique_ptr<block_device_t> device, fat32_fsdata_t* fs_data);
bool fat32_find_node(fat32_fsdata_t* fs_data, const char* path, size_t size, uint32_t cluster, fat32_node_t* out_node);
bool fat32_directory_exists(fat32_fsdata_t* fs_data, const char* path);
bool fat32_file_exists(fat32_fsdata_t* fs_data, const char* path);
bool fat32_read(fat32_fsdata_t* fs_data, const char* path, uint8_t** out_data, size_t* out_size);
bool fat32_list_directory(fat32_fsdata_t* fs_data, const char* path, std::dynamic_array<fat32_node_t>* out_nodes);

#endif // __FAT32_HPP__