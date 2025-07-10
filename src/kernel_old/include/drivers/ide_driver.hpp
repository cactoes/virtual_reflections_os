//==========================================
/// @file       ide_driver.hpp
/// @brief      ata, atapi, drive drivers
//==========================================

#pragma once

#ifndef __IDE_DRIVER_HPP__
#define __IDE_DRIVER_HPP__

#define IDE_SECTOR_SIZE     2048

#define IDE_REG_DATA            0
#define IDE_REG_ERROR_FEATURES  1
#define IDE_REG_SECCOUNT        2
#define IDE_REG_LBA_LOW         3
#define IDE_REG_LBA_MID         4
#define IDE_REG_LBA_HIGH        5
#define IDE_REG_DEVICE          6
#define IDE_REG_COMMAND_STATUS  7

#define IDE_DEVICE_MASTER       0xA0
#define IDE_DEVICE_SLAVE        0xB0

#define IDE_CMD_IDENTIFY        0xEC
#define IDE_CMD_PACKET          0xA0
#define IDE_CMD_IDENTIFY_PACKET 0xA1

#define IDE_STATUS_ERR          0x01
#define IDE_STATUS_DRQ          0x08
#define IDE_STATUS_SRV          0x10
#define IDE_STATUS_DF           0x20
#define IDE_STATUS_RDY          0x40
#define IDE_STATUS_BSY          0x80

#define IDE_CTRL_DISABLE_IRQ    0x02

#define ATAPI_CMD_READ_CAPACITY 0x25

#include "common.hpp"
#include "vector.hpp"
#include "drivers/pci_driver.hpp"
#include "file_systems/vfs.hpp"

enum class ide_channel_name_t {
    NONE = 0,
    PRIMARY,
    SECONDARY
};

enum class ide_drive_type_t {
    NONE = 0,
    MASTER,
    SLAVE
};

struct ide_channel_t {
    uint16_t io_base;
    uint16_t ctrl_base;
    uint16_t master;
    ide_channel_name_t channel_name;
};

struct ata_drive_t {
    bool is_atapi;
    ide_channel_name_t channel_name;
    ide_drive_type_t type;
    uint16_t io_base;
    uint16_t ctrl_base;

    char model[41];
    char serial[21];
    char firmware[9];

    uint64_t lba;
    uint64_t capacity;
    uint64_t logical_sector_size;
    uint64_t physical_sector_size;
};

int ide_init(pci_device_info_t* ide_pci_device, vector<ata_drive_t>* drives);

int ide_ata_identify_device(ata_drive_t* drive, uint16_t io_base, bool slave);
int ide_atapi_identify_device(ata_drive_t* drive, uint16_t io_base, bool slave);

int ide_identify(uint16_t io_base, bool slave, uint16_t* out_buf);
int ide_identify_packet(uint16_t io_base, bool slave, uint16_t* out_buf);

int ide_atapi_read(ata_drive_t* drive, uint32_t lba, uint8_t* buffer);

inline int atapi_read(drive_t* drive, uint32_t lba, void* buffer, size_t* size) {
    if (*size != IDE_SECTOR_SIZE)
        return 1;

    return ide_atapi_read((ata_drive_t*)drive->device, lba, (uint8_t*)buffer);
}

inline int atapi_write(drive_t* drive, uint32_t lba, void* buffer, size_t* size) {
    return 1;
}

#endif // __IDE_DRIVER_HPP__