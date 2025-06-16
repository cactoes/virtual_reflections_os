#include "drivers/ide_driver.hpp"
#include "debug.hpp"
#include "cpu.hpp"

extern void decode_string(const uint16_t* src, int word_count, char* dest, int max_len);

bool ide_atapi_packet(uint16_t io_base, bool is_slave, const uint8_t* packet, uint8_t* buffer, uint16_t buffer_size) {
    cpu_outb(io_base + IDE_REG_DEVICE, is_slave ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER);

    while (cpu_inb(io_base + IDE_REG_COMMAND_STATUS) & IDE_STATUS_BSY) {}

    cpu_outb(io_base + IDE_REG_LBA_MID, (uint8_t)(buffer_size & 0xFF));
    cpu_outb(io_base + IDE_REG_LBA_HIGH, (uint8_t)((buffer_size >> 8) & 0xFF));
    cpu_outb(io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_PACKET);

    while (true) {
        uint8_t status = cpu_inb(io_base + IDE_REG_COMMAND_STATUS);
        
        if (status & IDE_STATUS_DRQ)
            break;
        
        if (status & IDE_STATUS_ERR)
            return false;
    }

    for (int i = 0; i < 12; i += 2) {
        uint16_t w = (packet[i + 1] << 8) | packet[i];
        cpu_outw(io_base + IDE_REG_DATA, w);
    }

    while (true) {
        uint8_t status = cpu_inb(io_base + IDE_REG_COMMAND_STATUS);
        if (status & IDE_STATUS_DRQ)
            break;

        if (status & IDE_STATUS_ERR)
            return false;

        if (!(status & IDE_STATUS_BSY))
            continue;
    }

    for (int i = 0; i < buffer_size; i += 2) {
        uint16_t word = cpu_inw(io_base + IDE_REG_DATA);
        buffer[i] = word & 0xFF;
        
        if (i + 1 < buffer_size)
            buffer[i + 1] = word >> 8;
    }

    while (true) {
        uint8_t status = cpu_inb(io_base + IDE_REG_COMMAND_STATUS);

        if (!(status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
            break;

        if (status & IDE_STATUS_ERR)
            return false;
    }

    return true;
}

int ide_init(pci_device_info_t* ide_pci_device, vector<ata_drive_t>* drives) {
    ide_channel_t channels[] {
        {
            .io_base = (uint16_t)((ide_pci_device->bar0_address & 0xFFFFFFFC) ? (ide_pci_device->bar0_address & 0xFFFFFFFC) : 0x1F0),
            .ctrl_base = (uint16_t)((ide_pci_device->bar1_address & 0xFFFFFFFC) ? (ide_pci_device->bar1_address & 0xFFFFFFFC) : 0x3F6),
            .master = (uint16_t)((ide_pci_device->bar4_address & 0xFFFFFFFC) + 0),
            .channel_name = ide_channel_name_t::PRIMARY
        },
        {
            .io_base = (uint16_t)((ide_pci_device->bar2_address & 0xFFFFFFFC) ? (ide_pci_device->bar2_address & 0xFFFFFFFC) : 0x170),
            .ctrl_base = (uint16_t)((ide_pci_device->bar3_address & 0xFFFFFFFC) ? (ide_pci_device->bar3_address & 0xFFFFFFFC) : 0x376),
            .master = (uint16_t)((ide_pci_device->bar4_address & 0xFFFFFFFC) + 8),
            .channel_name = ide_channel_name_t::SECONDARY
        }
    };

    for (int i = 0; i < 2; i++) {
        ide_channel_t* chan = &channels[i];

        for (int drive = 0; drive < 2; drive++) {
            bool is_slave = (drive == 1);

            cpu_outb(chan->io_base + IDE_REG_DEVICE, is_slave ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER);
            cpu_outb(chan->ctrl_base, IDE_CTRL_DISABLE_IRQ);

            cpu_outb(chan->io_base + IDE_REG_ERROR_FEATURES, 0); 
            cpu_outb(chan->io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_IDENTIFY);

            uint8_t status = cpu_inb(chan->io_base + IDE_REG_COMMAND_STATUS);
            if (status == 0)
                continue;

            while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
                status = cpu_inb(chan->io_base + IDE_REG_COMMAND_STATUS);

            uint8_t cl = cpu_inb(chan->io_base + IDE_REG_LBA_MID);
            uint8_t ch = cpu_inb(chan->io_base + IDE_REG_LBA_HIGH);

            if (cl == 0x14 && ch == 0xEB) {
                ata_drive_t ata_drive {
                    .is_atapi = true,
                    .channel_name = chan->channel_name,
                    .type = is_slave ? ide_drive_type_t::SLAVE : ide_drive_type_t::MASTER,
                    .io_base = chan->io_base,
                    .ctrl_base = chan->ctrl_base,
                };

                ide_atapi_identify_device(&ata_drive, chan->io_base, is_slave);

                drives->insert_back(ata_drive);
            } else if (cl == 0 && ch == 0) {
                ata_drive_t ata_drive {
                    .is_atapi = false,
                    .channel_name = chan->channel_name,
                    .type = is_slave ? ide_drive_type_t::SLAVE : ide_drive_type_t::MASTER,
                    .io_base = chan->io_base,
                    .ctrl_base = chan->ctrl_base,
                };

                ide_ata_identify_device(&ata_drive, chan->io_base, is_slave);
                
                drives->insert_back(ata_drive);
            }
        }
    }
    
    return 0;
}

int ide_identify(uint16_t io_base, bool slave, uint16_t* out_buf) {
    cpu_outb(io_base + IDE_REG_DEVICE, 0xA0 | (slave ? 0x10 : 0x00));

    cpu_outb(io_base + IDE_REG_SECCOUNT, 0);
    cpu_outb(io_base + IDE_REG_LBA_LOW, 0);
    cpu_outb(io_base + IDE_REG_LBA_MID, 0);
    cpu_outb(io_base + IDE_REG_LBA_HIGH, 0);

    cpu_outb(io_base + IDE_REG_COMMAND_STATUS, 0xEC);

    uint8_t status = cpu_inb(io_base + IDE_REG_COMMAND_STATUS);
    if (status == 0)
        return 1;

    while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
        status = cpu_inb(io_base + IDE_REG_COMMAND_STATUS);

    for (int i = 0; i < 256; ++i)
        out_buf[i] = cpu_inw(io_base + 0);

    return 0;
}

int ide_identify_packet(uint16_t io_base, bool slave, uint16_t* out_buf) {
    cpu_outb(io_base + IDE_REG_DEVICE, (slave ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER));

    cpu_outb(io_base + IDE_REG_SECCOUNT, 0);
    cpu_outb(io_base + IDE_REG_LBA_LOW, 0);
    cpu_outb(io_base + IDE_REG_LBA_MID, 0);
    cpu_outb(io_base + IDE_REG_LBA_HIGH, 0);

    cpu_outb(io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_IDENTIFY_PACKET);

    uint8_t status = cpu_inb(io_base + IDE_REG_COMMAND_STATUS);
    if (status == 0)
        return 1;

    while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
        status = cpu_inb(io_base + IDE_REG_COMMAND_STATUS);

    for (int i = 0; i < 256; ++i)
        out_buf[i] = cpu_inw(io_base + 0);

    return 0;
}

int ide_ata_identify_device(ata_drive_t* drive, uint16_t io_base, bool slave) {
    uint16_t buffer[256] {};
    if (ide_identify_packet(io_base, slave, buffer) != 0)
        return 1;

    decode_string(&buffer[27], 20, drive->model, sizeof(drive->model));
    decode_string(&buffer[10], 10, drive->serial, sizeof(drive->serial));
    decode_string(&buffer[23], 4, drive->firmware, sizeof(drive->firmware));

    drive->lba = ((uint32_t)buffer[61] << 16) | buffer[60];

    if ((buffer[83] & (1 << 10)) && (buffer[100] || buffer[101] || buffer[102] || buffer[103])) {
        drive->lba =
            ((uint64_t)buffer[103] << 48) |
            ((uint64_t)buffer[102] << 32) |
            ((uint64_t)buffer[101] << 16) |
            ((uint64_t)buffer[100]);
    }

    drive->logical_sector_size = ((uint32_t)buffer[118] << 16) | buffer[117];
    if (drive->logical_sector_size == 0) drive->logical_sector_size = 512;

    drive->capacity = drive->lba * (uint64_t)drive->logical_sector_size;

    return 0;
}

int ide_atapi_identify_device(ata_drive_t* drive, uint16_t io_base, bool slave) {
    uint16_t buffer[256] {};
    if (ide_identify_packet(io_base, slave, buffer) != 0)
        return 1;

    decode_string(&buffer[27], 20, drive->model, sizeof(drive->model));
    decode_string(&buffer[10], 10, drive->serial, sizeof(drive->serial));
    decode_string(&buffer[23], 4, drive->firmware, sizeof(drive->firmware));

    uint8_t packet[12] = {};
    packet[0] = 0x25;

    uint8_t buffer2[8] = {};
    if (!ide_atapi_packet(io_base, slave, packet, buffer2, sizeof(buffer2)))
        return 2;

    uint32_t last_lba = (buffer2[0] << 24) | (buffer2[1] << 16) | (buffer2[2] << 8) | buffer2[3];
    uint32_t block_size = (buffer2[4] << 24) | (buffer2[5] << 16) | (buffer2[6] << 8) | buffer2[7];

    drive->lba = last_lba + 1;
    drive->logical_sector_size = block_size;
    drive->capacity = (uint64_t)drive->lba * block_size;

    return 0;
}

int ide_atapi_read(ata_drive_t* drive, uint32_t lba, uint8_t* buffer) {
    cpu_outb(drive->io_base + IDE_REG_DEVICE, drive->type == ide_drive_type_t::SLAVE ? IDE_DEVICE_SLAVE : IDE_DEVICE_MASTER);

    cpu_outb(drive->io_base + IDE_REG_ERROR_FEATURES, 0);
    cpu_outb(drive->io_base + IDE_REG_LBA_MID, 0);
    cpu_outb(drive->io_base + IDE_REG_LBA_HIGH, 8);

    cpu_outb(drive->io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_PACKET);

    while (true) {
        uint8_t status = cpu_inb(drive->io_base + IDE_REG_COMMAND_STATUS);
        if (!(status & IDE_STATUS_BSY) && (status & IDE_STATUS_DRQ))
            break;
    }

    uint8_t packet[12] = {0};
    packet[0] = 0x28; // read
    packet[1] = 0;
    packet[2] = (lba >> 24) & 0xFF;
    packet[3] = (lba >> 16) & 0xFF;
    packet[4] = (lba >> 8) & 0xFF;
    packet[5] = (lba >> 0) & 0xFF;
    packet[6] = 0;
    packet[7] = 1; // 1 sector
    packet[8] = 0;
    packet[9] = 0;
    packet[10] = 0;
    packet[11] = 0;

    for (int i = 0; i < 6; i++) {
        uint16_t w = ((uint16_t)packet[i * 2 + 1] << 8) | packet[i * 2];
        cpu_outw(drive->io_base, w);
    }

    while (true) {
        uint8_t status = cpu_inb(drive->io_base + IDE_REG_COMMAND_STATUS);
        if (status & IDE_STATUS_DRQ)
            break;
        if (status & IDE_STATUS_ERR)
            return 1;
    }

    for (int i = 0; i < IDE_SECTOR_SIZE / 2; i++) {
        uint16_t w = cpu_inw(drive->io_base);
        buffer[i * 2 + 0] = w & 0xFF;
        buffer[i * 2 + 1] = w >> 8;
    }
    
    cpu_inb(drive->ctrl_base);
    return 0;
}