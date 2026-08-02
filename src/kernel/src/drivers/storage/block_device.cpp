#include "drivers/storage/block_device.hpp"
#include "drivers/storage/ide.hpp"
#include "drivers/storage/ahci.hpp"

bool block_read_sized(block_device_t* device, u64 lba, u8* buffer, size_t size) {
    if (!device || !buffer)
        return false;

    if (size % device->block_size != 0)
        return false;

    const u64 lba_count = (size + device->block_size - 1) / device->block_size;

    for (size_t i = 0; i < lba_count; i++) {
        if (!block_read(device, lba + i, buffer + (i * device->block_size))) {
            return false;
        }
    }

    return true;
}

bool block_read(block_device_t* device, u64 lba, u8* buffer) {
    if (!device || !buffer)
        return false;

    switch (device->type) {
        case block_device_type_t::IDE:
            return ide_read((ide_device_t*)device->disk_device, device->start_lba + lba, buffer, device->block_size);
        case block_device_type_t::AHCI:
            return ahci_read((ahci_device_t*)device->disk_device, device->start_lba + lba, buffer, device->block_size);
        default:
            return false;
    }

    return false;
}

const char* block_device_get_model(block_device_t* device) {
    if (!device)
        return nullptr;

    switch (device->type) {
        case block_device_type_t::IDE:
            return ((ide_device_t*)device->disk_device)->meta.model;
        case block_device_type_t::AHCI:
            return ((ahci_device_t*)device->disk_device)->meta.model;
        default:
            return nullptr;
    }

    return nullptr;
}

const char* block_device_get_serial(block_device_t* device) {
    if (!device)
        return nullptr;

    switch (device->type) {
        case block_device_type_t::IDE:
            return ((ide_device_t*)device->disk_device)->meta.serial;
        case block_device_type_t::AHCI:
            return ((ahci_device_t*)device->disk_device)->meta.serial;
        default:
            return nullptr;
    }

    return nullptr;
}

const char* block_device_get_firmware(block_device_t* device) {
    if (!device)
        return nullptr;

    switch (device->type) {
        case block_device_type_t::IDE:
            return ((ide_device_t*)device->disk_device)->meta.firmware;
        case block_device_type_t::AHCI:
            return ((ahci_device_t*)device->disk_device)->meta.firmware;
        default:
            return nullptr;
    }

    return nullptr;
}

u64 block_device_get_drive_capacity(block_device_t* device) {
    if (!device)
        return 0;

    switch (device->type) {
        case block_device_type_t::IDE:
            return ((ide_device_t*)device->disk_device)->capacity;
        case block_device_type_t::AHCI:
            return ((ahci_device_t*)device->disk_device)->capacity;
        default:
            return 0;
    }

    return 0;
}