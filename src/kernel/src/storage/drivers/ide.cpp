#include "storage/drivers/ide.hpp"
#include "arch/generic.hpp"
#include "std/string.hpp"

bool wait_ide_status(uint16_t io_base, uint8_t mask_set, uint8_t mask_clear) {
    while (true) {
        uint8_t status = in_port<uint8_t>(io_base + IDE_REG_COMMAND_STATUS);

        if ((status & mask_set) == mask_set && (status & mask_clear) == 0)
            return true;

        if (status & IDE_STATUS_ERR)
            return false;
    }

    // ?
    return false;
}

bool ide_send_identify(ide_device_t* device, uint16_t* buffer) {
    out_port<uint8_t>(device->channel.io_base + IDE_REG_DEVICE, (device->type == ide_type_t::MASTER ? IDE_DEVICE_MASTER : IDE_DEVICE_SLAVE));
    out_port<uint8_t>(device->channel.io_base + IDE_REG_SECCOUNT, 0);
    out_port<uint8_t>(device->channel.io_base + IDE_REG_LBA_LOW, 0);
    out_port<uint8_t>(device->channel.io_base + IDE_REG_LBA_MID, 0);
    out_port<uint8_t>(device->channel.io_base + IDE_REG_LBA_HIGH, 0);
    out_port<uint8_t>(device->channel.io_base + IDE_REG_COMMAND_STATUS, (device->is_atapi ? IDE_CMD_IDENTIFY_PACKET : IDE_CMD_IDENTIFY));

    uint8_t status = in_port<uint8_t>(device->channel.io_base + IDE_REG_COMMAND_STATUS);
    if (status == 0)
        return false;

    while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
        status = in_port<uint8_t>(device->channel.io_base + IDE_REG_COMMAND_STATUS);

    if (!(status & IDE_STATUS_DRQ))
        return false;

    for (size_t i = 0; i < 256; i++)
        buffer[i] = in_port<uint16_t>(device->channel.io_base + IDE_REG_DATA);

    return true;
}

bool ide_ata_load_capacity(ide_device_t* device, uint16_t* identify_buffer) {
    if (!device || !identify_buffer)
        return false;

    device->lba_count = ((uint32_t)identify_buffer[61] << 16) | identify_buffer[60];
    if ((identify_buffer[83] & (1 << 10)) && (identify_buffer[100] || identify_buffer[101] || identify_buffer[102] || identify_buffer[103])) {
        device->lba_count =
            ((uint64_t)identify_buffer[103] << 48) |
            ((uint64_t)identify_buffer[102] << 32) |
            ((uint64_t)identify_buffer[101] << 16) |
            ((uint64_t)identify_buffer[100]);
    }

    device->logical_sector_size = ((uint32_t)identify_buffer[118] << 16) | identify_buffer[117];
    if (device->logical_sector_size == 0)
        device->logical_sector_size = 512;

    device->capacity = device->lba_count * device->logical_sector_size;

    return true;
}

bool ide_atapi_send_packet(ide_device_t* device, const uint8_t* packet, uint8_t* buffer, size_t size) {
    if (!device || !packet || !buffer)
        return false;

    out_port<uint8_t>(device->channel.io_base + IDE_REG_DEVICE, device->type == ide_type_t::MASTER ? IDE_DEVICE_MASTER : IDE_DEVICE_SLAVE);

    if (!wait_ide_status(device->channel.io_base, 0, IDE_STATUS_BSY))
        return false;

    out_port<uint8_t>(device->channel.io_base + IDE_REG_LBA_MID, (uint8_t)(size & 0xFF));
    out_port<uint8_t>(device->channel.io_base + IDE_REG_LBA_HIGH, (uint8_t)((size >> 8) & 0xFF));
    out_port<uint8_t>(device->channel.io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_PACKET);

    if (!wait_ide_status(device->channel.io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY))
        return false;

    for (size_t i = 0; i < 12; i += 2) {
        uint16_t part = (packet[i + 1] << 8) | packet[i];
        out_port<uint16_t>(device->channel.io_base + IDE_REG_DATA, part);
    }

    if (!wait_ide_status(device->channel.io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY))
        return false;

    for (size_t i = 0; i < size; i += 2) {
        uint16_t word = in_port<uint16_t>(device->channel.io_base + IDE_REG_DATA);
        buffer[i] = word & 0xFF;

        if (i + 1 < size)
            buffer[i + 1] = word >> 8;
    }

    if (!wait_ide_status(device->channel.io_base, 0, IDE_STATUS_BSY | IDE_STATUS_DRQ))
        return false;

    return true;
}

bool ide_atapi_load_capacity(ide_device_t* device) {
    if (!device)
        return false;

    uint8_t packet[12] {};
    packet[0] = ATAPI_CMD_READ_CAPACITY;

    uint8_t buffer[8] {};
    if (!ide_atapi_send_packet(device, packet, buffer, sizeof(buffer)))
        return false;

    device->lba_count = ((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]) + 1;
    device->logical_sector_size = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    device->capacity = device->lba_count * device->logical_sector_size;    

    return true;
}

bool ide_atapi_read(ide_device_t* device, uint64_t lba, uint8_t* buffer) {
    constexpr uint64_t sector_count = 1;

    if (!device)
        return false;

    if (lba >= device->lba_count)
        return false;

    uint8_t packet[12] {};
    packet[0] = 0x28; // read
    packet[1] = 0;
    packet[2] = (lba >> 24) & 0xFF;
    packet[3] = (lba >> 16) & 0xFF;
    packet[4] = (lba >> 8) & 0xFF;
    packet[5] = (lba >> 0) & 0xFF;
    packet[6] = 0x00;
    packet[7] = (sector_count >> 8) & 0xFF;
    packet[8] = (sector_count >> 0) & 0xFF;
    packet[9] = 0x00;
    packet[10] = 0x00;
    packet[11] = 0x00;

    if (!ide_atapi_send_packet(device, packet, buffer, device->logical_sector_size))
        return false;

    return true;
}

bool ide_ata_read(ide_device_t* device, uint64_t lba, uint8_t* buffer) {
    return false;
}

bool ide_init(const pci_device_t* device, std::dynamic_array<ide_device_t>* device_list) {
    if (!device)
        return false;

    const auto bar0 = pci_read_bar(device, 0);
    const auto bar1 = pci_read_bar(device, 1);
    const auto bar2 = pci_read_bar(device, 2);
    const auto bar3 = pci_read_bar(device, 3);
    const auto bar4 = pci_read_bar(device, 4);

    const ide_channel_t channels[] {
        {
            .io_base = (uint16_t)((bar0 & 0xFFFFFFFC) ? (bar0 & 0xFFFFFFFC) : IDE_DEFAULT_PRIMARY_IO_BASE),
            .ctrl_base = (uint16_t)((bar1 & 0xFFFFFFFC) ? (bar1 & 0xFFFFFFFC) : IDE_DEFAULT_PRIMARY_CTRL_BASE),
            .master = (uint16_t)((bar4 & 0xFFFFFFFC) + 0),
            .channel_type = ide_channel_type_t::PRIMARY
        },
        {
            .io_base = (uint16_t)((bar2 & 0xFFFFFFFC) ? (bar2 & 0xFFFFFFFC) : IDE_DEFAULT_SECONDARY_IO_BASE),
            .ctrl_base = (uint16_t)((bar3 & 0xFFFFFFFC) ? (bar3 & 0xFFFFFFFC) : IDE_DEFAULT_SECONDARY_CTRL_BASE),
            .master = (uint16_t)((bar4 & 0xFFFFFFFC) + 8),
            .channel_type = ide_channel_type_t::SECONDARY
        }
    };

    for (const auto& channel : channels) {
        for (int drive = 0; drive < 2; drive++) {
            const ide_type_t type = (drive == 0) ? ide_type_t::MASTER : ide_type_t::SLAVE;

            out_port<uint8_t>(channel.io_base + IDE_REG_DEVICE, type == ide_type_t::MASTER ? IDE_DEVICE_MASTER : IDE_DEVICE_SLAVE);
            out_port<uint8_t>(channel.ctrl_base, IDE_CTRL_DISABLE_IRQ);

            out_port<uint8_t>(channel.io_base + IDE_REG_ERROR_FEATURES, 0); 
            out_port<uint8_t>(channel.io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_IDENTIFY);

            uint8_t status = in_port<uint8_t>(channel.io_base + IDE_REG_COMMAND_STATUS);
            if (status == 0)
                continue;

            while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
                status = in_port<uint8_t>(channel.io_base + IDE_REG_COMMAND_STATUS);

            uint8_t cl = in_port<uint8_t>(channel.io_base + IDE_REG_LBA_MID);
            uint8_t ch = in_port<uint8_t>(channel.io_base + IDE_REG_LBA_HIGH);

            ide_device_t ide_device {};
            ide_device.channel = channel;
            ide_device.type = type;
            ide_device.is_atapi = cl == ATAPI_SIG_LBA_MID && ch == ATAPI_SIG_LBA_HIGH;

            if (!ide_device_init(&ide_device))
                continue;

            device_list->insert_back(ide_device);
        }
    }

    return true;
}

bool ide_device_init(ide_device_t* device) {
    if (!device)
        return false;

    uint16_t buffer[256] {};
    if (!ide_send_identify(device, buffer))
        return false;

    str_unpack_be16(&buffer[27], 20, device->meta.model, sizeof(device->meta.model));
    str_unpack_be16(&buffer[10], 10, device->meta.serial, sizeof(device->meta.serial));
    str_unpack_be16(&buffer[23], 4, device->meta.firmware, sizeof(device->meta.firmware));

    return device->is_atapi ? ide_atapi_load_capacity(device) : ide_ata_load_capacity(device, buffer);
}

bool ide_read(ide_device_t* device, uint64_t lba, uint8_t* buffer, size_t size) {
    if (!device || !buffer)
        return false;

    if (device->logical_sector_size != size)
        return false;

    return device->is_atapi ? ide_atapi_read(device, lba, buffer) : ide_ata_read(device, lba, buffer);
}

bool ide_write(ide_device_t* device) {
    if (!device)
        return false;

    // TODO @since 24/01/2026 -- 03:06

    return false;
}