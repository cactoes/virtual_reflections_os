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
    uint32_t lba_end;
} PACKED;

struct mbr_t {
    char bootstrap[440];
    uint32_t signature;
    uint16_t reserved;
    mbr_entry_t partitions[MBR_PARTITIONS];
    uint16_t signatre;
} PACKED;

#endif // __MBR_HPP__