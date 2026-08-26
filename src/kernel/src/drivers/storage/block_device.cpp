#include "drivers/storage/block_device.hpp"

bool block_device_read(block_device_t* block_device, u64 lba, u8* buffer, u64 size) {
    return block_device->interface->read(
        block_device->disk_data,
        block_device->start_lba + lba,
        buffer,
        size);
}

bool block_device_read_sized(block_device_t* device, u64 lba, u8* buffer, u64 size) {
    if (!device || !buffer)
        return false;

    if (size % device->block_size != 0)
        return false;

    const u64 lba_count = (size + device->block_size - 1) / device->block_size;

    for (size_t i = 0; i < lba_count; i++)
        if (!block_device_read(device, lba + i, buffer + (i * device->block_size), device->block_size))
            return false;

    return true;
}