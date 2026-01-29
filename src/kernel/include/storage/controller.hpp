//==========================================
/// @file       controller.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __CONTROLLER_HPP__
#define __CONTROLLER_HPP__

#define MBR_PARTITIONS 4

#include "common.hpp"
#include "drivers/pcie.hpp"

struct sc_mbr_entry_t {
    uint8_t attributes;
    char chs_start[3];
    uint8_t partition_type;
    char chs_end[3];
    uint32_t lba_start;
    uint32_t lba_end;
} PACKED;

struct sc_mbr_t {
    char bootstrap[440];
    uint32_t signature;
    uint16_t reserved;
    sc_mbr_entry_t partitions[MBR_PARTITIONS];
    uint16_t signatre;
} PACKED;

enum class sc_device_type_t {
    UNKOWN = 0,
    IDE,
    AHCI
};

struct sc_device_t {
    uint64_t total_sectors;
    uint64_t size_per_sector;

    sc_mbr_entry_t mbr_entry;
    bool has_mbr;

    void* data;
};

struct block_device_t {
    void* disk_device;
    sc_device_type_t type;
    uint64_t start_lba;
    uint64_t end_lba;
    size_t block_size;
};

bool testidk(const pci_device_t* ide_device);

#endif // __CONTROLLER_HPP__