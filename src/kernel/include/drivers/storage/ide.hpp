//==========================================
/// @file       ide.hpp
/// @brief      ide / ata / atapi device driver
//==========================================

#pragma once

#ifndef __DRIVERS_STORAGE_IDE_HPP__
#define __DRIVERS_STORAGE_IDE_HPP__

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

#include "drivers/storage/storage.hpp"
#include "drivers/pcie.hpp"
#include "utils/vector.hpp"

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

struct ide_device_t : storage_driver_api_t {
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

int ide_init(const pci_device_t* p_pcie_device, linked_list<ide_device_t>* p_ide_devices);

int ide_send_identify(ide_device_t* p_device, uint16_t* p_out_buf);

int ide_ata_identify_device(ide_device_t* p_device);
int ide_atapi_identify_device(ide_device_t* p_device);

int ide_read(ide_device_t* p_device, uint32_t lba, uint8_t* p_buffer);
int ide_write(ide_device_t* p_device, uint32_t lba, uint8_t* p_buffer);

inline int ide_storage_api_read(storage_driver_api_t* p_device, uint32_t lba, uint8_t* p_buffer, size_t size) {
    if (size % IDE_SECTOR_SIZE != 0)
        return 1;

    size_t written_size = 0;
    while (written_size < size) {
        if (ide_read((ide_device_t*)p_device, lba, p_buffer + written_size) != 0)
            return 2;

        written_size += IDE_SECTOR_SIZE;
        lba++;
    }
    
    return 0;
}

inline int ide_storage_api_write(storage_driver_api_t* p_device, uint32_t lba, uint8_t* p_buffer, size_t size) {
    // TODO @since 21/07/2025 -- 21:32
    return 1;
}

#endif // __DRIVERS_STORAGE_IDE_HPP__