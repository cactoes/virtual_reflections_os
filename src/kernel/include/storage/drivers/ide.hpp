//==========================================
/// @file       ide.hpp
/// @brief      ide storage driver
//==========================================

#pragma once

#ifndef __IDE_HPP__
#define __IDE_HPP__

#define IDE_SECTOR_SIZE                     2048

#define IDE_REG_DATA                        0x00
#define IDE_REG_ERROR_FEATURES              0x01
#define IDE_REG_SECCOUNT                    0x02
#define IDE_REG_LBA_LOW                     0x03
#define IDE_REG_LBA_MID                     0x04
#define IDE_REG_LBA_HIGH                    0x05
#define IDE_REG_DEVICE                      0x06
#define IDE_REG_COMMAND_STATUS              0x07

#define IDE_DEVICE_MASTER                   0xA0
#define IDE_DEVICE_SLAVE                    0xB0

#define IDE_CMD_IDENTIFY                    0xEC
#define IDE_CMD_PACKET                      0xA0
#define IDE_CMD_IDENTIFY_PACKET             0xA1
#define ATAPI_CMD_READ_CAPACITY             0x25

#define IDE_STATUS_ERR                      0x01
#define IDE_STATUS_DRQ                      0x08
#define IDE_STATUS_SRV                      0x10
#define IDE_STATUS_DF                       0x20
#define IDE_STATUS_RDY                      0x40
#define IDE_STATUS_BSY                      0x80

#define IDE_CTRL_DISABLE_IRQ                0x02

#define IDE_DEFAULT_PRIMARY_IO_BASE         0x1F0
#define IDE_DEFAULT_PRIMARY_CTRL_BASE       0x3F6
#define IDE_DEFAULT_SECONDARY_IO_BASE       0x170
#define IDE_DEFAULT_SECONDARY_CTRL_BASE     0x376

#define ATAPI_SIG_LBA_MID  0x14
#define ATAPI_SIG_LBA_HIGH 0xEB

#include "common.hpp"
#include "drivers/pcie.hpp"
#include "std/array.hpp"

enum class ide_channel_type_t {
    NONE = 0,
    PRIMARY,
    SECONDARY
};

enum class ide_type_t {
    NONE = 0,
    MASTER,
    SLAVE
};

struct ide_channel_t {
    uint16_t io_base;
    uint16_t ctrl_base;
    uint16_t master;
    ide_channel_type_t channel_type;
};

struct ide_device_t {
    bool is_atapi;
    ide_type_t type;
    ide_channel_t channel;

    uint64_t lba_count;
    uint64_t capacity;
    uint64_t logical_sector_size;
    uint64_t physical_sector_size;

    struct {
        char model[41];
        char serial[21];
        char firmware[9];
    } meta;
};

bool ide_init(const pci_device_t* device, std::dynamic_array<ide_device_t>* device_list);
bool ide_device_init(ide_device_t* device);
bool ide_read(ide_device_t* device, uint64_t lba, uint8_t* buffer, size_t size);
bool ide_write(ide_device_t* device);
bool is_ide_device(const pci_device_t* device);

#endif // __IDE_HPP__