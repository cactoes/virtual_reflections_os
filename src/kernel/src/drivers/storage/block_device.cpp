#include "drivers/storage/block_device.hpp"
#include "drivers/storage/ide.hpp"
#include "drivers/storage/ahci.hpp"

bool block_read_sized(block_device_t* device, uint64_t lba, uint8_t* buffer, size_t size) {
    if (!device || !buffer)
        return false;

    if (size % device->block_size != 0)
        return false;

    const uint64_t lba_count = (size + device->block_size - 1) / device->block_size;

    for (size_t i = 0; i < lba_count; i++) {
        if (!block_read(device, lba + i, buffer + (i * device->block_size))) {
            return false;
        }
    }

    return true;
}

bool block_read(block_device_t* device, uint64_t lba, uint8_t* buffer) {
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