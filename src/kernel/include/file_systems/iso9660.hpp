//==========================================
/// @file       iso9660.hpp
/// @brief      iso file system helper
//==========================================

#pragma once

#ifndef __ISO9660_HPP__
#define __ISO9660_HPP__

#include "common.hpp"

struct iso9660_volume_descriptor_t {
    uint8_t type;
    char identifier[5];
    uint8_t version;
    uint8_t unused1;
    char system_identifier[32];
    char volume_identifier[32];
    uint8_t unused2[8];
    uint32_t volume_space_size_le;
    uint32_t volume_space_size_be;
    uint8_t unused[2048 - 40];
} PACKED;

struct iso9660_dir_record_t {
    uint8_t length;
    uint8_t ext_attr_length;
    uint32_t extent_lba_le;
    uint32_t extent_lba_be;
    uint32_t data_length_le;
    uint32_t data_length_be;
    uint8_t recording_date[7];
    uint8_t file_flags;
    uint8_t file_unit_size;
    uint8_t interleave_gap_size;
    uint16_t volume_sequence_le;
    uint16_t volume_sequence_be;
    uint8_t name_len;
    char name[];
} PACKED;

#endif // __ISO9660_HPP__