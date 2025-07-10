//==========================================
/// @file       fat32.hpp
/// @brief      fat32 file system
/// @note       NOT MY IMPLEMENTATION / MOSTLY COPIED FROM THE INTERNET
//==========================================

#pragma once

#ifndef __FAT32_HPP__
#define __FAT32_HPP__

#define FAT32_DIRATTR_READ_ONLY     0x01
#define FAT32_DIRATTR_HIDDEN        0x02
#define FAT32_DIRATTR_SYSTEM        0x04
#define FAT32_DIRATTR_VOLUME_ID     0x08
#define FAT32_DIRATTR_DIRECTORY     0x10
#define FAT32_DIRATTR_ARCHIVE       0x20
#define FAT32_DIRATTR_LFN           (FAT32_DIRATTR_READ_ONLY | FAT32_DIRATTR_HIDDEN | FAT32_DIRATTR_SYSTEM | FAT32_DIRATTR_VOLUME_ID)

#define FAT32_EOC32                 0x0FFFFFF8u
#define FAT32_SECTOR_SIZE           512

#include "common.hpp"
#include "file_systems/vfs.hpp"

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
    uint32_t volumeID;
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
    uint32_t size;
    bool is_directory;
};

struct fat32_fs_data_t {
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t first_fat_sector;
    uint32_t sectors_per_fat;
    uint32_t first_data_sector;
    uint32_t root_cluster;
};

static inline uint16_t fat_date_year(fat32_date_t d)  { return 1980 + ((d >> 9) & 0x7F); }
static inline uint8_t fat_date_month(fat32_date_t d) { return (d >> 5) & 0x0F; }
static inline uint8_t fat_date_day(fat32_date_t d)   { return d & 0x1F; }

static inline uint8_t fat_time_hour(fat32_time_t t)  { return (t >> 11) & 0x1F; }
static inline uint8_t fat_time_min(fat32_time_t t)   { return (t >> 5)  & 0x3F; }
static inline uint8_t fat_time_sec(fat32_time_t t)   { return (t & 0x1F) * 2; }

int fat32_drive_init(drive_t* drive, fs_t* fs);
int fat32_drive_deinit(fs_t* fs);
int fat32_read_file(fs_t*, drive_t* drive, const char* file_path, void** file_data, size_t* size);
int fat32_write_file(fs_t*, drive_t* drive, const char* file_path, void* file_data, size_t* size);

#endif // __FAT32_HPP__