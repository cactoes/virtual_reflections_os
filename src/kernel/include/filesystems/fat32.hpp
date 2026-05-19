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

#define FAT32_DATE_YEAR(d)   (u16)(1980 + ((d >> 9) & 0x7F))
#define FAT32_DATE_MONTH(d)  (u8)((d >> 5) & 0x0F)
#define FAT32_DATE_DAY(d)    (u8)(d & 0x1F)

#define FAT32_TIME_HOUR(t)   (u8)((t >> 11) & 0x1F)
#define FAT32_TIME_MIN(t)    (u8)((t >> 5)  & 0x3F)
#define FAT32_TIME_SEC(t)    (u8)((t & 0x1F) * 2)

#include "common.hpp"
#include "drivers/storage/block_device.hpp"
#include "std/array.hpp"
#include "std/pointer.hpp"

typedef u16 fat32_date_t;
typedef u16 fat32_time_t;

struct fat32_bpb_t {
    char jmp[3];
    char oem_name[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 fat_count;
    u16 root_entry_count;
    u16 total_sectors;
    u8 media_type;
    u16 fat_size_16;
    u16 sectors_per_track;
    u16 num_heads;
    u32 hidden_sectors;
    u32 total_sectors_large;
} PACKED;

struct fat32_bpb_extended_t {
    fat32_bpb_t bpb;

    u32 fat_size_32;
    u16 flags;
    u16 version;
    u32 root_cluster;
    u16 fs_info;
    u16 backup_boot_sector;
    u8 reserved[12];
    u8 drive_number;
    u8 reserved1;
    u8 boot_signature;
    u32 volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[420];
    u32 boot_parition_signature;
} PACKED;

struct fat32_dir_entry_t {
    char name[11];
    u8 attributes;
    u8 nt_reserved;
    u8 creation_time_tenths;
    fat32_time_t creation_time;
    fat32_date_t creation_date;
    fat32_date_t last_access_date;
    u16 first_cluster_high;
    fat32_time_t write_time;
    fat32_date_t write_date;
    u16 first_cluster_low;
    u32 file_size;
} PACKED;

struct fat32_lfn_entry_t {
    u8 order;
    u16 name1[5];
    u8 attr;
    u8 type;
    u8 checksum;
    u16 name2[6];
    u16 zero;
    u16 name3[2];
} PACKED;

struct fat32_node_t {
    u32 first_cluster;
    u64 size;
    bool is_directory;
    char name[256];
};

struct fat32_fsdata_t {
    std::unique_ptr<block_device_t> block_device;
    u64 volume_size;
    fat32_node_t root_node;

    struct {
        u16 bytes_per_sector;
        u8  sectors_per_cluster;
        u32 sectors_per_fat;
        u32 first_fat_sector;
        u32 first_data_sector;
        u32 root_cluster;
    } layout;
};

bool fat32_validate(u8* buffer, size_t size);

bool fat32_init(std::unique_ptr<block_device_t> device, fat32_fsdata_t* fs_data);
bool fat32_find_node(fat32_fsdata_t* fs_data, const char* path, size_t size, u32 cluster, fat32_node_t* out_node);
bool fat32_directory_exists(fat32_fsdata_t* fs_data, const char* path);
bool fat32_file_exists(fat32_fsdata_t* fs_data, const char* path);
bool fat32_read(fat32_fsdata_t* fs_data, const char* path, u8** out_data, size_t* out_size);
bool fat32_list_directory(fat32_fsdata_t* fs_data, const char* path, std::dynamic_array<fat32_node_t>* out_nodes);

#endif // __FAT32_HPP__