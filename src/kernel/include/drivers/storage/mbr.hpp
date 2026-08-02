#pragma once

#ifndef __MBR_HPP__
#define __MBR_HPP__

#define MBR_PARTITIONS 4

#include "common.hpp"

struct mbr_entry_t {
    u8 attributes;
    char chs_start[3];
    u8 partition_type;
    char chs_end[3];
    u32 lba_start;
    u32 sector_count;
} __packed;

struct mbr_t {
    char bootstrap[440];
    u32 disk_id;
    u16 reserved;
    mbr_entry_t partitions[MBR_PARTITIONS];
    u16 signature;
} __packed;

bool mbr_is_entry_valid(const mbr_entry_t* entry);
bool is_mbr(const u8* data, size_t size);

#endif // __MBR_HPP__