#pragma once

#ifndef __MBR_HPP__
#define __MBR_HPP__

#define MBR_PARTITIONS 4

#include "common.hpp"

struct mbr_entry_t {
    uint8_t attributes;
    char chs_start[3];
    uint8_t partition_type;
    char chs_end[3];
    uint32_t lba_start;
    uint32_t sector_count;
} PACKED;

struct mbr_t {
    char bootstrap[440];
    uint32_t disk_id;
    uint16_t reserved;
    mbr_entry_t partitions[MBR_PARTITIONS];
    uint16_t signature;
} PACKED;

bool mbr_is_entry_valid(const mbr_entry_t* entry);
bool is_mbr(const uint8_t* data, size_t size);

#endif // __MBR_HPP__