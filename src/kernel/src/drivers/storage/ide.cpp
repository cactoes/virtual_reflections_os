#include "drivers/storage/ide.hpp"
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

bool ide_atapi_send_packet(ide_device_t* p_device, const uint8_t* p_packet, uint8_t* p_buffer, uint16_t buffer_size) {
    out_port<uint8_t>(p_device->io_base + IDE_REG_DEVICE, p_device->type == ide_drive_type_t::SLAVE ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER);

    if (!wait_ide_status(p_device->io_base, 0, IDE_STATUS_BSY))
        return false;

    out_port<uint8_t>(p_device->io_base + IDE_REG_LBA_MID, (uint8_t)(buffer_size & 0xFF));
    out_port<uint8_t>(p_device->io_base + IDE_REG_LBA_HIGH, (uint8_t)((buffer_size >> 8) & 0xFF));
    out_port<uint8_t>(p_device->io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_PACKET);

    if (!wait_ide_status(p_device->io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY))
        return false;

    for (size_t i = 0; i < 12; i += 2) {
        uint16_t packet = (p_packet[i + 1] << 8) | p_packet[i];
        out_port<uint16_t>(p_device->io_base + IDE_REG_DATA, packet);
    }

    if (!wait_ide_status(p_device->io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY))
        return false;

    for (size_t i = 0; i < buffer_size; i += 2) {
        uint16_t word = in_port<uint16_t>(p_device->io_base + IDE_REG_DATA);
        p_buffer[i] = word & 0xFF;
        
        if (i + 1 < buffer_size)
            p_buffer[i + 1] = word >> 8;
    }

    if (!wait_ide_status(p_device->io_base, 0, IDE_STATUS_BSY | IDE_STATUS_DRQ))
        return false;

    return true;
}

int ide_init(const pci_device_t* p_pcie_device, linked_list<ide_device_t>* p_ide_devices) {
    const auto bar0 = pci_read_bar(p_pcie_device, 0);
    const auto bar1 = pci_read_bar(p_pcie_device, 1);
    const auto bar2 = pci_read_bar(p_pcie_device, 2);
    const auto bar3 = pci_read_bar(p_pcie_device, 3);
    const auto bar4 = pci_read_bar(p_pcie_device, 4);

    ide_channel_t channels[] {
        {
            .io_base = (uint16_t)((bar0 & 0xFFFFFFFC) ? (bar0 & 0xFFFFFFFC) : IDE_DEFAULT_PRIMARY_IO_BASE),
            .ctrl_base = (uint16_t)((bar1 & 0xFFFFFFFC) ? (bar1 & 0xFFFFFFFC) : IDE_DEFAULT_PRIMARY_CTRL_BASE),
            .master = (uint16_t)((bar4 & 0xFFFFFFFC) + 0),
            .channel_name = ide_channel_name_t::PRIMARY
        },
        {
            .io_base = (uint16_t)((bar2 & 0xFFFFFFFC) ? (bar2 & 0xFFFFFFFC) : IDE_DEFAULT_SECONDARY_IO_BASE),
            .ctrl_base = (uint16_t)((bar3 & 0xFFFFFFFC) ? (bar3 & 0xFFFFFFFC) : IDE_DEFAULT_SECONDARY_CTRL_BASE),
            .master = (uint16_t)((bar4 & 0xFFFFFFFC) + 8),
            .channel_name = ide_channel_name_t::SECONDARY
        }
    };

    for (int i = 0; i < 2; i++) {
        const ide_channel_t* channel = &channels[i];

        for (int drive = 0; drive < 2; drive++) {
            const bool is_slave = (drive == 1);

            out_port<uint8_t>(channel->io_base + IDE_REG_DEVICE, is_slave ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER);
            out_port<uint8_t>(channel->ctrl_base, IDE_CTRL_DISABLE_IRQ);

            out_port<uint8_t>(channel->io_base + IDE_REG_ERROR_FEATURES, 0); 
            out_port<uint8_t>(channel->io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_IDENTIFY);

            uint8_t status = in_port<uint8_t>(channel->io_base + IDE_REG_COMMAND_STATUS);
            if (status == 0)
                continue;

            while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
                status = in_port<uint8_t>(channel->io_base + IDE_REG_COMMAND_STATUS);

            uint8_t cl = in_port<uint8_t>(channel->io_base + IDE_REG_LBA_MID);
            uint8_t ch = in_port<uint8_t>(channel->io_base + IDE_REG_LBA_HIGH);

            ide_device_t ide_device {};

            ide_device.channel_name = channel->channel_name;
            ide_device.type = is_slave ? ide_drive_type_t::SLAVE : ide_drive_type_t::MASTER;
            ide_device.io_base = channel->io_base;
            ide_device.ctrl_base = channel->ctrl_base;

            if (cl == 0x14 && ch == 0xEB) {
                ide_device.is_atapi = true;
                ide_atapi_identify_device(&ide_device);
                p_ide_devices->insert_back(ide_device);
            } else if (cl == 0 && ch == 0) {
                ide_device.is_atapi = false;
                ide_ata_identify_device(&ide_device);
                p_ide_devices->insert_back(ide_device);
            }
        }
    }

    return 1;
}

int ide_send_identify(ide_device_t* p_device, uint16_t* p_out_buf) {
    out_port<uint8_t>(p_device->io_base + IDE_REG_DEVICE, (p_device->type == ide_drive_type_t::SLAVE ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER));
    out_port<uint8_t>(p_device->io_base + IDE_REG_SECCOUNT, 0);
    out_port<uint8_t>(p_device->io_base + IDE_REG_LBA_LOW, 0);
    out_port<uint8_t>(p_device->io_base + IDE_REG_LBA_MID, 0);
    out_port<uint8_t>(p_device->io_base + IDE_REG_LBA_HIGH, 0);
    out_port<uint8_t>(p_device->io_base + IDE_REG_COMMAND_STATUS, (p_device->is_atapi ? IDE_CMD_IDENTIFY_PACKET : IDE_CMD_IDENTIFY));

    uint8_t status = in_port<uint8_t>(p_device->io_base + IDE_REG_COMMAND_STATUS);
    if (status == 0)
        return 1;

    while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
        status = in_port<uint8_t>(p_device->io_base + IDE_REG_COMMAND_STATUS);

    if (!(status & IDE_STATUS_DRQ))
        return 2;

    for (size_t i = 0; i < 256; i++)
        p_out_buf[i] = in_port<uint16_t>(p_device->io_base + IDE_REG_DATA);

    return 0;
}

int ide_ata_identify_device(ide_device_t* p_device) {
    uint16_t buffer[256] {};
    if (ide_send_identify(p_device, buffer) != 0)
        return 1;

    str_unpack_be16(&buffer[27], 20, p_device->model, sizeof(p_device->model));
    str_unpack_be16(&buffer[10], 10, p_device->serial, sizeof(p_device->serial));
    str_unpack_be16(&buffer[23], 4, p_device->firmware, sizeof(p_device->firmware));

    p_device->lba = ((uint32_t)buffer[61] << 16) | buffer[60];

    if ((buffer[83] & (1 << 10)) && (buffer[100] || buffer[101] || buffer[102] || buffer[103])) {
        p_device->lba =
            ((uint64_t)buffer[103] << 48) |
            ((uint64_t)buffer[102] << 32) |
            ((uint64_t)buffer[101] << 16) |
            ((uint64_t)buffer[100]);
    }

    p_device->logical_sector_size = ((uint32_t)buffer[118] << 16) | buffer[117];
    if (p_device->logical_sector_size == 0) p_device->logical_sector_size = 512;

    p_device->capacity = p_device->lba * (uint64_t)p_device->logical_sector_size;

    return 0;
}

int ide_atapi_identify_device(ide_device_t* p_device) {
    uint16_t identify_buffer[256] {};
    if (ide_send_identify(p_device, identify_buffer) != 0)
        return 1;

    str_unpack_be16(&identify_buffer[27], 20, p_device->model, sizeof(p_device->model));
    str_unpack_be16(&identify_buffer[10], 10, p_device->serial, sizeof(p_device->serial));
    str_unpack_be16(&identify_buffer[23], 4, p_device->firmware, sizeof(p_device->firmware));

    uint8_t packet[12] = {};
    packet[0] = 0x25;

    uint8_t identify_buffer2[8] = {};
    if (!ide_atapi_send_packet(p_device, packet, identify_buffer2, sizeof(identify_buffer2)))
        return 2;

    uint32_t last_lba = (identify_buffer2[0] << 24) | (identify_buffer2[1] << 16) | (identify_buffer2[2] << 8) | identify_buffer2[3];
    uint32_t block_size = (identify_buffer2[4] << 24) | (identify_buffer2[5] << 16) | (identify_buffer2[6] << 8) | identify_buffer2[7];

    p_device->lba = last_lba + 1;
    p_device->logical_sector_size = block_size;
    p_device->capacity = (uint64_t)p_device->lba * block_size;

    return 0;
}

void create_atapi_struct_read(uint8_t p_packet[12], uint64_t lba, uint16_t sector_count) {
    memzero(p_packet, 12);

    p_packet[0] = 0x28; // read
    p_packet[1] = 0;
    p_packet[2] = (lba >> 24) & 0xFF;
    p_packet[3] = (lba >> 16) & 0xFF;
    p_packet[4] = (lba >> 8) & 0xFF;
    p_packet[5] = (lba >> 0) & 0xFF;
    p_packet[6] = 0x00;
    p_packet[7] = (sector_count >> 8) & 0xFF;
    p_packet[8] = (sector_count >> 0) & 0xFF;
    p_packet[9] = 0x00;
    p_packet[10] = 0x00;
    p_packet[11] = 0x00;
}

int atapi_read(ide_device_t* p_device, uint32_t lba, uint8_t* p_buffer) {
    out_port<uint8_t>(p_device->io_base + IDE_REG_DEVICE, p_device->type == ide_drive_type_t::SLAVE ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER);
    out_port<uint8_t>(p_device->io_base + IDE_REG_ERROR_FEATURES, 0);
    out_port<uint8_t>(p_device->io_base + IDE_REG_LBA_MID, 0);
    out_port<uint8_t>(p_device->io_base + IDE_REG_LBA_HIGH, 8);
    out_port<uint8_t>(p_device->io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_PACKET);

    if (!wait_ide_status(p_device->io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY))
        return 1;

    uint8_t packet[12];
    create_atapi_struct_read(packet, lba, 1);

    for (int i = 0; i < 6; i++) {
        uint16_t w = ((uint16_t)packet[i * 2 + 1] << 8) | packet[i * 2];
        out_port<uint16_t>(p_device->io_base, w);
    }

    if (!wait_ide_status(p_device->io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY))
        return 1;

    for (int i = 0; i < IDE_SECTOR_SIZE / 2; i++) {
        uint16_t w = in_port<uint16_t>(p_device->io_base);
        p_buffer[i * 2 + 0] = w & 0xFF;
        p_buffer[i * 2 + 1] = w >> 8;
    }
    
    in_port<uint8_t>(p_device->ctrl_base);
    return 0;
}

ide_storage_driver_t::ide_storage_driver_t(ide_device_t* device) {
    this->device = device;
}

bool ide_storage_driver_t::read(uint32_t lba, uint8_t* buffer, size_t size) {
    if (size % IDE_SECTOR_SIZE != 0)
        return false;

    size_t written_size = 0;
    while (written_size < size) {
        if (device->is_atapi) {
            if (atapi_read(device, lba, buffer + written_size) != 0)
                return false;
        } else {
            return false;
        }

        written_size += IDE_SECTOR_SIZE;
        lba++;
    }
    
    return true;
}

bool ide_storage_driver_t::write(uint32_t lba, uint8_t* buffer, size_t size) {
    // TODO @since 10/10/2025 -- 20:20
    return 1;
}

size_t ide_storage_driver_t::get_block_size() {
    return IDE_SECTOR_SIZE;
}

void ide_storage_driver_t::set_root_lba(uint64_t lba) {
    root_lba = lba;
}

uint64_t ide_storage_driver_t::get_root_lba() {
    return root_lba;
}

storage_info_t ide_storage_driver_t::get_storage_info() const {
    storage_info_t info {};
    info.model = device->model;
    info.serial = device->serial;
    info.firmare = device->firmware;
    info.capacity = device->capacity;
    return info;
}