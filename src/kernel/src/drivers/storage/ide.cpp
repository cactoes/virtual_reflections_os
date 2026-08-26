#include "drivers/storage/ide.hpp"

#include "std/string.hpp"
// TODO @since 21/05/2026 -- 22:29
// you know the drill
#include "arch/amd64/port.hpp"

bool wait_ide_status(u16 io_base, u8 mask_set, u8 mask_clear) {
    while (true) {
        u8 status = amd64_in_port8(io_base + IDE_REG_COMMAND_STATUS);

        if ((status & mask_set) == mask_set && (status & mask_clear) == 0)
            return true;

        if (status & IDE_STATUS_ERR)
            return false;
    }

    // ?
    return false;
}

bool ide_send_identify(ide_device_t* device, u16* buffer) {
    amd64_out_port8(device->channel.io_base + IDE_REG_DEVICE, (device->type == ide_type_t::MASTER ? IDE_DEVICE_MASTER : IDE_DEVICE_SLAVE));
    amd64_out_port8(device->channel.io_base + IDE_REG_SECCOUNT, 0);
    amd64_out_port8(device->channel.io_base + IDE_REG_LBA_LOW, 0);
    amd64_out_port8(device->channel.io_base + IDE_REG_LBA_MID, 0);
    amd64_out_port8(device->channel.io_base + IDE_REG_LBA_HIGH, 0);
    amd64_out_port8(device->channel.io_base + IDE_REG_COMMAND_STATUS, (device->is_atapi ? IDE_CMD_IDENTIFY_PACKET : IDE_CMD_IDENTIFY));

    u8 status = amd64_in_port8(device->channel.io_base + IDE_REG_COMMAND_STATUS);
    if (status == 0)
        return false;

    while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
        status = amd64_in_port8(device->channel.io_base + IDE_REG_COMMAND_STATUS);

    if (!(status & IDE_STATUS_DRQ))
        return false;

    for (size_t i = 0; i < 256; i++)
        buffer[i] = amd64_in_port16(device->channel.io_base + IDE_REG_DATA);

    return true;
}

bool ide_ata_load_capacity(ide_device_t* device, u16* identify_buffer) {
    if (!device || !identify_buffer)
        return false;

    device->lba_count = ((u32)identify_buffer[61] << 16) | identify_buffer[60];
    if ((identify_buffer[83] & (1 << 10)) && (identify_buffer[100] || identify_buffer[101] || identify_buffer[102] || identify_buffer[103])) {
        device->lba_count =
            ((u64)identify_buffer[103] << 48) |
            ((u64)identify_buffer[102] << 32) |
            ((u64)identify_buffer[101] << 16) |
            ((u64)identify_buffer[100]);
    }

    device->logical_sector_size = ((u32)identify_buffer[118] << 16) | identify_buffer[117];
    if (device->logical_sector_size == 0)
        device->logical_sector_size = 512;

    device->capacity = device->lba_count * device->logical_sector_size;

    return true;
}

bool ide_atapi_send_packet(ide_device_t* device, const u8* packet, u8* buffer, size_t size) {
    if (!device || !packet || !buffer)
        return false;

    amd64_out_port8(device->channel.io_base + IDE_REG_DEVICE, device->type == ide_type_t::MASTER ? IDE_DEVICE_MASTER : IDE_DEVICE_SLAVE);

    spinlock_lock(&device->spinlock);

    if (!wait_ide_status(device->channel.io_base, 0, IDE_STATUS_BSY)) {
        spinlock_unlock(&device->spinlock);
        return false;
    }

    amd64_out_port8(device->channel.io_base + IDE_REG_LBA_MID, (u8)(size & 0xFF));
    amd64_out_port8(device->channel.io_base + IDE_REG_LBA_HIGH, (u8)((size >> 8) & 0xFF));
    amd64_out_port8(device->channel.io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_PACKET);

    if (!wait_ide_status(device->channel.io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY)) {
        spinlock_unlock(&device->spinlock);
        return false;
    }

    for (size_t i = 0; i < 12; i += 2) {
        u16 part = (packet[i + 1] << 8) | packet[i];
        amd64_out_port16(device->channel.io_base + IDE_REG_DATA, part);
    }

    if (!wait_ide_status(device->channel.io_base, IDE_STATUS_DRQ, IDE_STATUS_BSY)) {
        spinlock_unlock(&device->spinlock);
        return false;
    }

    for (size_t i = 0; i < size; i += 2) {
        u16 word = amd64_in_port16(device->channel.io_base + IDE_REG_DATA);
        buffer[i] = word & 0xFF;

        if (i + 1 < size)
            buffer[i + 1] = word >> 8;
    }

    if (!wait_ide_status(device->channel.io_base, 0, IDE_STATUS_BSY | IDE_STATUS_DRQ)) {
        spinlock_unlock(&device->spinlock);
        return false;
    }

    spinlock_unlock(&device->spinlock);
    return true;
}

bool ide_atapi_load_capacity(ide_device_t* device) {
    if (!device)
        return false;

    u8 packet[12] {};
    packet[0] = ATAPI_CMD_READ_CAPACITY;

    u8 buffer[8] {};
    if (!ide_atapi_send_packet(device, packet, buffer, sizeof(buffer)))
        return false;

    device->lba_count = ((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]) + 1;
    device->logical_sector_size = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    device->capacity = device->lba_count * device->logical_sector_size;    

    return true;
}

bool ide_atapi_read(ide_device_t* device, u64 lba, u8* buffer) {
    constexpr u64 sector_count = 1;

    if (!device)
        return false;

    if (lba >= device->lba_count)
        return false;

    u8 packet[12] {};
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

bool ide_ata_read(ide_device_t* device, u64 lba, u8* buffer) {
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
            .io_base = (u16)((bar0 & 0xFFFFFFFC) ? (bar0 & 0xFFFFFFFC) : IDE_DEFAULT_PRIMARY_IO_BASE),
            .ctrl_base = (u16)((bar1 & 0xFFFFFFFC) ? (bar1 & 0xFFFFFFFC) : IDE_DEFAULT_PRIMARY_CTRL_BASE),
            .master = (u16)((bar4 & 0xFFFFFFFC) + 0),
            .channel_type = ide_channel_type_t::PRIMARY
        },
        {
            .io_base = (u16)((bar2 & 0xFFFFFFFC) ? (bar2 & 0xFFFFFFFC) : IDE_DEFAULT_SECONDARY_IO_BASE),
            .ctrl_base = (u16)((bar3 & 0xFFFFFFFC) ? (bar3 & 0xFFFFFFFC) : IDE_DEFAULT_SECONDARY_CTRL_BASE),
            .master = (u16)((bar4 & 0xFFFFFFFC) + 8),
            .channel_type = ide_channel_type_t::SECONDARY
        }
    };

    for (const auto& channel : channels) {
        for (int drive = 0; drive < 2; drive++) {
            const ide_type_t type = (drive == 0) ? ide_type_t::MASTER : ide_type_t::SLAVE;

            amd64_out_port8(channel.io_base + IDE_REG_DEVICE, type == ide_type_t::MASTER ? IDE_DEVICE_MASTER : IDE_DEVICE_SLAVE);
            amd64_out_port8(channel.ctrl_base, IDE_CTRL_DISABLE_IRQ);

            amd64_out_port8(channel.io_base + IDE_REG_ERROR_FEATURES, 0); 
            amd64_out_port8(channel.io_base + IDE_REG_COMMAND_STATUS, IDE_CMD_IDENTIFY);

            u8 status = amd64_in_port8(channel.io_base + IDE_REG_COMMAND_STATUS);
            if (status == 0)
                continue;

            while ((status & IDE_STATUS_BSY) && !(status & IDE_STATUS_DRQ))
                status = amd64_in_port8(channel.io_base + IDE_REG_COMMAND_STATUS);

            u8 cl = amd64_in_port8(channel.io_base + IDE_REG_LBA_MID);
            u8 ch = amd64_in_port8(channel.io_base + IDE_REG_LBA_HIGH);

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

    u16 buffer[256] {};
    if (!ide_send_identify(device, buffer))
        return false;

    str_unpack_be16(&buffer[27], 20, device->meta.model, sizeof(device->meta.model));
    str_unpack_be16(&buffer[10], 10, device->meta.serial, sizeof(device->meta.serial));
    str_unpack_be16(&buffer[23], 4, device->meta.firmware, sizeof(device->meta.firmware));

    return device->is_atapi ? ide_atapi_load_capacity(device) : ide_ata_load_capacity(device, buffer);
}

bool ide_read(ide_device_t* device, u64 lba, u8* buffer, size_t size) {
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

bool is_ide_device(const pci_device_t* device) {
    return device->class_info.class_code == 0x1 &&
           device->class_info.sub_class == 0x1;
}

u64 ide_get_sector_size(ide_device_t* device) {
    return device->logical_sector_size;
}

u64 ide_get_capacity(ide_device_t* device) {
    return device->capacity;
}

const disk_interface_t* get_ide_disk_interface() {
    static const disk_interface_t interface {
        .read = (decltype(disk_interface_t::read))ide_read,
        .write = nullptr,
        .get_sector_size = (decltype(disk_interface_t::get_sector_size))ide_get_sector_size,
        .get_capacity = (decltype(disk_interface_t::get_capacity))ide_get_capacity,
        .get_model = [](void* disk_data) -> const char* { return ((ide_device_t*)disk_data)->meta.model; },
        .get_serial = [](void* disk_data) -> const char* { return ((ide_device_t*)disk_data)->meta.serial; },
        .get_firmware = [](void* disk_data) -> const char* { return ((ide_device_t*)disk_data)->meta.firmware; }
    };

    return &interface;
}