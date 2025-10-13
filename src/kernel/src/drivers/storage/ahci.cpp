#include "drivers/storage/ahci.hpp"
#include "memory/vmem.hpp"
#include "memory/heap.hpp"
#include "arch/generic.hpp"

heap_t global_ahci_dma_heap {};

ahci_storage_driver_t::ahci_storage_driver_t(ahci_drive_t* drive) {
    this->drive = drive;
}

bool ahci_storage_driver_t::read(uint32_t lba, uint8_t* buffer, size_t size) {
    if (size % drive->logical_sector_size != 0)
        return false;

    switch (drive->port->sig) {
        case AHCI_PORT_SIG_SATA: {
            size_t written_size = 0;
            while (written_size < size) {
                // sector count 1?
                if (ahci_sata_read(drive, lba, 1, buffer + written_size) != 0)
                    return false;

                written_size += drive->logical_sector_size;
                lba++;
            }

            return true;
        }
        default:
            return false;
    }

    return false;
}

bool ahci_storage_driver_t::write(uint32_t lba, uint8_t* buffer, size_t size) {
    if (size % drive->logical_sector_size != 0)
        return false;

    switch (drive->port->sig) {
        case AHCI_PORT_SIG_SATA: {
            size_t written_size = 0;
            while (written_size < size) {
                // sector count 1?
                if (ahci_sata_write(drive, lba, 1, buffer + written_size) != 0)
                    return false;

                written_size += drive->logical_sector_size;
                lba++;
            }

            return true;
        }
        default:
            return false;
    }

    return false;
}

size_t ahci_storage_driver_t::get_block_size() {
    return drive->logical_sector_size;
}

void ahci_storage_driver_t::set_root_lba(uint64_t lba) {
    root_lba = lba;
}

uint64_t ahci_storage_driver_t::get_root_lba() {
    return root_lba;
}

void decode_string(const uint16_t* src, int word_count, char* dest, int max_len) {
    int pos = 0;
    for (int i = 0; i < word_count && pos + 1 < max_len; ++i) {
        dest[pos++] = (char)(src[i] >> 8);
        dest[pos++] = (char)(src[i] & 0xFF);
    }
    dest[pos] = '\0';
}

int ahci_init(const pci_device_t* pice_device, linked_list<ahci_drive_t>* device_list) {
    // still sketchy to just randomly get the pml4 table but whtv
    if (dma_heap_init(get_pml4(), &global_ahci_dma_heap, (void*)VMEM_AHCI_DMA, PAGE_SIZE_LARGE) != 0)
        return 1;

    const uint64_t mmio_addr_physical = (uint64_t)pci_read_bar(pice_device, 5);
    const uint64_t mmio_addr_physical_page = align_down(mmio_addr_physical, PAGE_SIZE_LARGE);
    const uint64_t mmio_addr_offset = mmio_addr_physical - mmio_addr_physical_page;

    if (!vmem_map_2mb_page(get_pml4(), (void*)VMEM_AHCI_MMIO, (void*)mmio_addr_physical_page))
        return 2;

    volatile hba_mem_t* hba = (volatile hba_mem_t*)((uint8_t*)VMEM_AHCI_MMIO + mmio_addr_offset);

    // for interupts enable:
    // hba->ghc |= AHCI_GHC_IE;
    hba->ghc |= AHCI_GHC_AE;

    for (int i = 0; i < 32; i++) {
        if (!(hba->pi & (1 << i)))
            continue;

        volatile hba_port_t* port = &hba->ports[i];

        uint8_t ssts_det = port->ssts & AHCI_SSTS_DET_MASK;
        uint8_t ssts_spd = (port->ssts & AHCI_SSTS_SPD_MASK) >> 4;
        uint8_t ssts_ipm = (port->ssts & AHCI_SSTS_IPM_MASK) >> 8;

        if (ssts_det != AHCI_DET_PHY_INITIALIZED || ssts_ipm != AHCI_IPM_ACTIVE)
            continue;

        ahci_drive_t drive {};
        drive.port = port;
        drive.was_setup = false;
        drive.clb = ahci_port_init(drive.port);
        if (!drive.clb)
            continue;

        switch (port->sig) {
            case AHCI_PORT_SIG_SATA: {
                drive.was_setup = ahci_sata_identify_device(&drive) == 0;
                break;
            }
            case AHCI_PORT_SIG_ATAPI: {
                drive.was_setup = ahci_atapi_identify_device(&drive) == 0;
                break;
            }
            default:
                break;
        }

        device_list->insert_back(drive);
    }

    return 0;
}

void* ahci_port_init(volatile hba_port_t* port) {
    port->cmd &= ~AHCI_PORT_CMD_ST;
    while (port->cmd & AHCI_PORT_CMD_CR);
    port->cmd &= ~AHCI_PORT_CMD_FRE;

    void* clb = dma_heap_alloc(&global_ahci_dma_heap, PAGE_SIZE, PAGE_SIZE);
    if (!clb)
        return nullptr;

    memzero(clb, PAGE_SIZE);
    port->clb = dma_get_physical_lower(&global_ahci_dma_heap, clb);
    port->clbu = dma_get_physical_upper(&global_ahci_dma_heap, clb);

    void* fb = dma_heap_alloc(&global_ahci_dma_heap, PAGE_SIZE, PAGE_SIZE);
    if (!fb) {
        dma_heap_free(&global_ahci_dma_heap, clb);
        return nullptr;
    }
    
    memzero(fb, PAGE_SIZE);
    port->fb = dma_get_physical_lower(&global_ahci_dma_heap, fb);
    port->fbu = dma_get_physical_upper(&global_ahci_dma_heap, fb);

    port->ie = 0;

    // for interrupts enable:
    // port->ie = 0xFFFFFFFF; // should end up at irq11?

    port->cmd |= AHCI_PORT_CMD_FRE | AHCI_PORT_CMD_ST;

    return clb;
}

int ahci_find_command_slot(volatile hba_port_t* port) {
    uint32_t slots = port->sact | port->ci;

    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0)
            return i;
    }

    return -1;
}

int ahci_sata_identify_device(ahci_drive_t* drive) {
    int slot = ahci_find_command_slot(drive->port);
    if (slot < 0)
        return 1;

    ahci_cmd_context_t ctx {};
    if (ahci_sata_prepare_command(&ctx, drive, 0, 1, ATA_CMD_IDENTIFY_DEVICE, false, ATA_DEV_DEFAULT, slot) != 0)
        return 2;

    drive->port->is = (uint32_t)-1;
    drive->port->ci |= (1 << slot);

    while (drive->port->ci & (1 << slot)) {}

    const uint16_t* data = (const uint16_t*)ctx.data_buffer;

    decode_string(&data[27], 20, drive->model, sizeof(drive->model));
    decode_string(&data[10], 10, drive->serial, sizeof(drive->serial));
    decode_string(&data[23], 4, drive->firmware, sizeof(drive->firmware));

    // lba28 fallback
    drive->lba = (uint32_t)data[60] | ((uint32_t)data[61] << 16);

    // check for 48bit lba
    if (data[83] & (1 << 10)) {
        drive->lba = ((uint64_t)data[100]) |
            ((uint64_t)data[101] << 16) |
            ((uint64_t)data[102] << 32) |
            ((uint64_t)data[103] << 48);
    }

    drive->logical_sector_size = (uint32_t)data[117] | ((uint32_t)data[118] << 16);
    if (drive->logical_sector_size == 0) drive->logical_sector_size = 512;

    drive->physical_sector_size = drive->logical_sector_size;
    uint16_t word106 = data[106];
    if (word106 & (1 << 13)) {
        uint8_t log2_multiple = word106 & 0x1F;
        drive->physical_sector_size = drive->logical_sector_size << log2_multiple;
    }

    drive->capacity = drive->lba * (uint64_t)drive->logical_sector_size;

    dma_heap_free(&global_ahci_dma_heap, ctx.cmdtable);
    dma_heap_free(&global_ahci_dma_heap, ctx.data_buffer);

    return 0;
}

int ahci_sata_prepare_command(ahci_cmd_context_t* ctx, ahci_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t ata_command, bool write, uint8_t fis_device, uint8_t slot) {
    ctx->slot = slot;

    ctx->cmdheader = (hba_cmd_header_t*)drive->clb;
    memzero(&ctx->cmdheader[slot], sizeof(hba_cmd_header_t));

    ctx->cmdheader[slot].cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    ctx->cmdheader[slot].w = write ? 1 : 0;
    ctx->cmdheader[slot].prdtl = 1;

    ctx->cmdtable = (hba_cmd_tbl_t*)dma_heap_alloc(&global_ahci_dma_heap, PAGE_SIZE, PAGE_SIZE);
    if (!ctx->cmdtable)
        return 1;

    memzero(ctx->cmdtable, PAGE_SIZE);

    ctx->cmdheader[slot].ctba = dma_get_physical_lower(&global_ahci_dma_heap, ctx->cmdtable);
    ctx->cmdheader[slot].ctbau = dma_get_physical_upper(&global_ahci_dma_heap, ctx->cmdtable);

    ctx->data_buffer = dma_heap_alloc(&global_ahci_dma_heap, PAGE_SIZE, PAGE_SIZE);
    if (!ctx->data_buffer) {
        dma_heap_free(&global_ahci_dma_heap, ctx->cmdtable);
        return 2;
    }

    memzero(ctx->data_buffer, PAGE_SIZE);

    ctx->cmdtable->prdt_entry[0].dba = dma_get_physical_lower(&global_ahci_dma_heap, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbau = dma_get_physical_upper(&global_ahci_dma_heap, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbc = (drive->logical_sector_size * sector_count) - 1;
    // enable this for interrupts:
    // ctx->cmdtable->prdt_entry[0].i = 0;

    ctx->fis = (fis_reg_h2d_t*)(&ctx->cmdtable->cfis);
    memzero(ctx->fis, sizeof(fis_reg_h2d_t));
    ctx->fis->fis_type = FIS_TYPE_REG_H2D;
    ctx->fis->c = 1;
    ctx->fis->command = ata_command;
    ctx->fis->device = fis_device;

    ctx->fis->lba0 = (uint8_t)(lba);
    ctx->fis->lba1 = (uint8_t)(lba >> 8);
    ctx->fis->lba2 = (uint8_t)(lba >> 16);
    ctx->fis->lba3 = (uint8_t)(lba >> 24);
    ctx->fis->lba4 = (uint8_t)(lba >> 32);
    ctx->fis->lba5 = (uint8_t)(lba >> 40);
    ctx->fis->countl = (uint8_t)(sector_count);
    ctx->fis->counth = (uint8_t)(sector_count >> 8);

    return 0;
}

int ahci_sata_read(ahci_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t* buffer) {
    int slot = ahci_find_command_slot(drive->port);
    if (slot < 0)
        return 1;

    ahci_cmd_context_t ctx {};
    if (ahci_sata_prepare_command(&ctx, drive, lba, sector_count, ATA_CMD_READ_DMA_EXT, false, ATA_DEV_LBA, slot) != 0)
        return 2;

    drive->port->is = (uint32_t)-1;
    drive->port->ci |= (1 << slot);

    while (drive->port->ci & (1 << slot))
        if (drive->port->is & AHCI_PORT_INT_TFES)
            return 2;

    while ((drive->port->tfd & (0x80 | 0x8)));

    if (drive->port->is & AHCI_PORT_INT_TFES)
		return 3;

    memcpy(buffer, ctx.data_buffer, drive->logical_sector_size);

    dma_heap_free(&global_ahci_dma_heap, ctx.cmdtable);
    dma_heap_free(&global_ahci_dma_heap, ctx.data_buffer);

    return 0;
}

int ahci_sata_write(ahci_drive_t* drive, uint64_t lba, uint16_t sector_count, const uint8_t* buffer) {
    int slot = ahci_find_command_slot(drive->port);
    if (slot < 0)
        return 1;

    ahci_cmd_context_t ctx {};
    ahci_sata_prepare_command(&ctx, drive, lba, sector_count, ATA_CMD_WRITE_DMA_EXT, true, ATA_DEV_LBA, slot);

    memcpy(ctx.data_buffer, buffer, sector_count * drive->logical_sector_size);

    drive->port->is = (uint32_t)-1;
    drive->port->ci |= (1 << slot);

    while (drive->port->ci & (1 << slot))
        if (drive->port->is & AHCI_PORT_INT_TFES)
            return 2;

    if (drive->port->is & AHCI_PORT_INT_TFES)
		return 3;

    dma_heap_free(&global_ahci_dma_heap, ctx.cmdtable);
    dma_heap_free(&global_ahci_dma_heap, ctx.data_buffer);

    return 0;
}

int ahci_atapi_identify_device(ahci_drive_t* drive) {
    int slot = ahci_find_command_slot(drive->port);
    if (slot < 0)
        return 1;

    ahci_cmd_context_t ctx {};
    uint8_t atapi_packet[12] = {};
    atapi_packet[0] = 0xA1;

    if (ahci_atapi_prepare_command(&ctx, drive, atapi_packet, sizeof(atapi_packet), 512, false, slot) != 0)
        return 2;

    drive->port->is = (uint32_t)-1;
    drive->port->ci |= (1 << slot);

    while (drive->port->ci & (1 << slot))
        if (drive->port->is & AHCI_PORT_INT_TFES)
            return 3;

    if (drive->port->is & AHCI_PORT_INT_TFES)
		return 4;

    const uint16_t* data = (const uint16_t*)ctx.data_buffer;

    decode_string(&data[10], 10, drive->model, sizeof(drive->model));
    decode_string(&data[23], 4, drive->serial, sizeof(drive->serial));
    decode_string(&data[27], 20, drive->firmware, sizeof(drive->firmware));

    // lba28 fallback
    drive->lba = (uint32_t)data[60] | ((uint32_t)data[61] << 16);

    // check for 48bit lba
    if (data[83] & (1 << 10)) {
        drive->lba = ((uint64_t)data[100]) |
            ((uint64_t)data[101] << 16) |
            ((uint64_t)data[102] << 32) |
            ((uint64_t)data[103] << 48);
    }

    drive->logical_sector_size = (uint32_t)data[117] | ((uint32_t)data[118] << 16);
    if (drive->logical_sector_size == 0) drive->logical_sector_size = 512;

    drive->physical_sector_size = drive->logical_sector_size;
    uint16_t word106 = data[106];
    if (word106 & (1 << 13)) {
        uint8_t log2_multiple = word106 & 0x1F;
        drive->physical_sector_size = drive->logical_sector_size << log2_multiple;
    }

    drive->capacity = drive->lba * (uint64_t)drive->logical_sector_size;

    dma_heap_free(&global_ahci_dma_heap, ctx.cmdtable);
    dma_heap_free(&global_ahci_dma_heap, ctx.data_buffer);

    return 0;
}

int ahci_atapi_prepare_command(ahci_cmd_context_t* ctx, ahci_drive_t* drive, const uint8_t* atapi_packet, size_t packet_size, uint16_t data_length, bool write, uint8_t slot) {
    ctx->slot = slot;

    ctx->cmdheader = (hba_cmd_header_t*)drive->clb;
    memzero(&ctx->cmdheader[slot], sizeof(hba_cmd_header_t));

    ctx->cmdheader[slot].cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    ctx->cmdheader[slot].w = write ? 1 : 0;
    ctx->cmdheader[slot].prdtl = 1;
    ctx->cmdheader[slot].a = 1;

    ctx->cmdtable = (hba_cmd_tbl_t*)dma_heap_alloc(&global_ahci_dma_heap, PAGE_SIZE, PAGE_SIZE);
    if (!ctx->cmdtable)
        return 1;

    memzero(ctx->cmdtable, PAGE_SIZE);

    ctx->cmdheader[slot].ctba = dma_get_physical_lower(&global_ahci_dma_heap, ctx->cmdtable);
    ctx->cmdheader[slot].ctbau = dma_get_physical_upper(&global_ahci_dma_heap, ctx->cmdtable);

    ctx->data_buffer = dma_heap_alloc(&global_ahci_dma_heap, PAGE_SIZE, PAGE_SIZE);
    if (!ctx->data_buffer) {
        dma_heap_free(&global_ahci_dma_heap, ctx->cmdtable);
        return 2;
    }

    memzero(ctx->data_buffer, PAGE_SIZE);

    ctx->cmdtable->prdt_entry[0].dba = dma_get_physical_lower(&global_ahci_dma_heap, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbau = dma_get_physical_upper(&global_ahci_dma_heap, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbc = data_length - 1;
    ctx->cmdtable->prdt_entry[0].i = 0;

    ctx->fis = (fis_reg_h2d_t*)(&ctx->cmdtable->cfis);
    memzero(ctx->fis, sizeof(fis_reg_h2d_t));
    ctx->fis->fis_type = FIS_TYPE_REG_H2D;
    ctx->fis->c = 1;
    ctx->fis->command = ATA_CMD_PACKET;
    ctx->fis->device = 0x00;

    memcpy(ctx->cmdtable->acmd, atapi_packet, packet_size);

    return 0;
}