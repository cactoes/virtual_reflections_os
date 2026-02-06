#include "storage/drivers/ahci.hpp"
#include "memory/vmem.hpp"
#include "std/string.hpp"

void* ahci_init_clb(ahci_device_t* device) {
    device->port->cmd &= ~AHCI_PORT_CMD_ST;
    while (device->port->cmd & AHCI_PORT_CMD_CR);
    device->port->cmd &= ~AHCI_PORT_CMD_FRE;

    void* clb = dma_heap_alloc(device->ahci_driver_ctx->dma, PAGE_SIZE, PAGE_SIZE);
    if (!clb)
        return nullptr;

    memzero(clb, PAGE_SIZE);
    device->port->clb = dma_get_physical_lower(device->ahci_driver_ctx->dma, clb);
    device->port->clbu = dma_get_physical_upper(device->ahci_driver_ctx->dma, clb);

    void* fb = dma_heap_alloc(device->ahci_driver_ctx->dma, PAGE_SIZE, PAGE_SIZE);
    if (!fb) {
        dma_heap_free(device->ahci_driver_ctx->dma, clb);
        return nullptr;
    }
    
    memzero(fb, PAGE_SIZE);
    device->port->fb = dma_get_physical_lower(device->ahci_driver_ctx->dma, fb);
    device->port->fbu = dma_get_physical_upper(device->ahci_driver_ctx->dma, fb);

    device->port->ie = 0;

    // for interrupts enable:
    // port->ie = 0xFFFFFFFF; // should end up at irq11?

    device->port->cmd |= AHCI_PORT_CMD_FRE | AHCI_PORT_CMD_ST;

    return clb;
}

int ahci_port_find_command_slot(volatile hba_port_t* port) {
    uint32_t slots = port->sact | port->ci;

    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0)
            return i;
    }

    return -1;
}

bool ahci_sata_prepare_command(ahci_cmd_context_t* ctx, ahci_device_t* device, uint64_t lba, uint64_t sector_count, uint8_t ata_command, bool write, uint8_t fis_device, uint8_t slot) {
    ctx->slot = slot;

    ctx->cmdheader = (hba_cmd_header_t*)device->clb;
    memzero(&ctx->cmdheader[slot], sizeof(hba_cmd_header_t));

    ctx->cmdheader[slot].cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    ctx->cmdheader[slot].w = write ? 1 : 0;
    ctx->cmdheader[slot].prdtl = 1;
    ctx->cmdheader[slot].prdbc = 0;

    ctx->cmdtable = (hba_cmd_tbl_t*)dma_heap_alloc(device->ahci_driver_ctx->dma, PAGE_SIZE, PAGE_SIZE);
    if (!ctx->cmdtable)
        return false;

    memzero(ctx->cmdtable, PAGE_SIZE);

    ctx->cmdheader[slot].ctba = dma_get_physical_lower(device->ahci_driver_ctx->dma, ctx->cmdtable);
    ctx->cmdheader[slot].ctbau = dma_get_physical_upper(device->ahci_driver_ctx->dma, ctx->cmdtable);

    ctx->data_buffer = dma_heap_alloc(device->ahci_driver_ctx->dma, PAGE_SIZE, PAGE_SIZE);
    if (!ctx->data_buffer) {
        dma_heap_free(device->ahci_driver_ctx->dma, ctx->cmdtable);
        return false;
    }

    memzero(ctx->data_buffer, PAGE_SIZE);

    ctx->cmdtable->prdt_entry[0].dba = dma_get_physical_lower(device->ahci_driver_ctx->dma, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbau = dma_get_physical_upper(device->ahci_driver_ctx->dma, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbc = (device->logical_sector_size * sector_count) - 1;
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

    return true;
}

bool ahci_sata_identify_device(ahci_device_t* device) {
    constexpr uint64_t sector_count = 1;

    if (!device)
        return false;

    int slot = ahci_port_find_command_slot(device->port);
    if (slot < 0)
        return false;

    ahci_cmd_context_t ctx {};
    if (!ahci_sata_prepare_command(&ctx, device, 0, sector_count, ATA_CMD_IDENTIFY_DEVICE, false, ATA_DEV_DEFAULT, slot) != 0)
        return false;

    device->port->is = (uint32_t)-1;
    device->port->ci |= (1 << slot);

    while (device->port->ci & (1 << slot)) {}

    const uint16_t* data = (const uint16_t*)ctx.data_buffer;

    str_unpack_be16(&data[27], 20, device->meta.model, sizeof(device->meta.model));
    str_unpack_be16(&data[10], 10, device->meta.serial, sizeof(device->meta.serial));
    str_unpack_be16(&data[23], 4, device->meta.firmware, sizeof(device->meta.firmware));

    // lba28 fallback
    device->lba_count = (uint32_t)data[60] | ((uint32_t)data[61] << 16);

    // check for 48bit lba
    if (data[83] & (1 << 10)) {
        device->lba_count = ((uint64_t)data[100]) |
            ((uint64_t)data[101] << 16) |
            ((uint64_t)data[102] << 32) |
            ((uint64_t)data[103] << 48);
    }

    device->logical_sector_size = (uint32_t)data[117] | ((uint32_t)data[118] << 16);
    if (device->logical_sector_size == 0) device->logical_sector_size = 512;

    device->physical_sector_size = device->logical_sector_size;
    uint16_t word106 = data[106];
    if (word106 & (1 << 13)) {
        uint8_t log2_multiple = word106 & 0x1F;
        device->physical_sector_size = device->logical_sector_size << log2_multiple;
    }

    device->capacity = device->lba_count * (uint64_t)device->logical_sector_size;

    dma_heap_free(device->ahci_driver_ctx->dma, ctx.cmdtable);
    dma_heap_free(device->ahci_driver_ctx->dma, ctx.data_buffer);

    return true;
}

bool ahci_sata_read(ahci_device_t* device, uint64_t lba, uint8_t* buffer) {
    constexpr uint64_t sector_count = 1;

    if (!device)
        return false;

    if (lba >= device->lba_count)
        return false;

    int slot = ahci_port_find_command_slot(device->port);
    if (slot < 0)
        return false;

    ahci_cmd_context_t ctx {};
    if (!ahci_sata_prepare_command(&ctx, device, lba, sector_count, ATA_CMD_READ_DMA_EXT, false, ATA_DEV_LBA, slot) != 0)
        return false;

    auto release_buffers = [](ahci_cmd_context_t* ctx, ahci_driver_ctx_t* ahci_driver_ctx) {
        if (!ctx || !ahci_driver_ctx)
            return;

        dma_heap_free(ahci_driver_ctx->dma, ctx->cmdtable);
        dma_heap_free(ahci_driver_ctx->dma, ctx->data_buffer);
    };

    device->port->is = (uint32_t)-1;
    device->port->ci |= (1 << slot);

    while (device->port->ci & (1 << slot)) {
        if (device->port->is & AHCI_PORT_INT_TFES) {
            release_buffers(&ctx, device->ahci_driver_ctx);
            return false;
        }
    }

    while ((device->port->tfd & (0x80 | 0x8)));

    if (device->port->is & AHCI_PORT_INT_TFES) {
        release_buffers(&ctx, device->ahci_driver_ctx);
		return false;
    }

    if (ctx.cmdheader[slot].prdbc == 0) {
        release_buffers(&ctx, device->ahci_driver_ctx);
        return false;
    }

    memcpy(buffer, ctx.data_buffer, device->logical_sector_size);
    release_buffers(&ctx, device->ahci_driver_ctx);
    return true;
}

bool ahci_init(const pci_device_t* device, ahci_driver_ctx_t* ahci_driver_ctx, std::dynamic_array<ahci_device_t>* device_list) {
    if (!device || !ahci_driver_ctx || !device_list)
        return false;

    ahci_driver_ctx->dma = dma_heap_manager_create_heap(get_global_dma_heap_manager(), PAGE_SIZE_LARGE);
    if (!ahci_driver_ctx->dma)
        return false;

    const uint64_t mmio_addr_physical = (uint64_t)pci_read_bar(device, 5);
    volatile hba_mem_t* hba = (volatile hba_mem_t*)vmem_map_mmio_region(get_global_dma_heap_manager()->pml4, (void*)mmio_addr_physical);
    if (!hba)
        return false;

    // enable for interrupts
    // hba->ghc |= AHCI_GHC_IE;
    hba->ghc |= AHCI_GHC_AE;

    for (int i = 0; i < 32; i++) {
        if (!(hba->pi & (1 << i)))
            continue;

        volatile hba_port_t* port = &hba->ports[i];

        uint8_t ssts_det = port->ssts & AHCI_SSTS_DET_MASK;
        // uint8_t ssts_spd = (port->ssts & AHCI_SSTS_SPD_MASK) >> 4;
        uint8_t ssts_ipm = (port->ssts & AHCI_SSTS_IPM_MASK) >> 8;

        if (ssts_det != AHCI_DET_PHY_INITIALIZED || ssts_ipm != AHCI_IPM_ACTIVE)
            continue;

        ahci_device_t ahci_device {};
        ahci_device.port = port;
        ahci_device.ahci_driver_ctx = ahci_driver_ctx;

        switch (port->sig) {
            case AHCI_PORT_SIG_SATA:
                ahci_device.type = ahci_device_type_t::SATA;
                break;
            default:
                ahci_device.type = ahci_device_type_t::UNKNOWN;
                break;
        }

        if (!ahci_device_init(&ahci_device))
            continue;

        device_list->insert_back(ahci_device);
    }

    return true;
}

bool ahci_device_init(ahci_device_t* device) {
    if (!device)
        return false;

    if (device->type == ahci_device_type_t::UNKNOWN)
        return false;

    device->clb = ahci_init_clb(device);
    if (!device->clb)
        return false;

    switch (device->type) {
        case ahci_device_type_t::SATA: return ahci_sata_identify_device(device);
        default: return false;
    }

    return false;
}

bool ahci_read(ahci_device_t* device, uint64_t lba, uint8_t* buffer, size_t size) {
    if (!device || !buffer)
        return false;

    if (device->logical_sector_size != size)
        return false;

    switch (device->type) {
        case ahci_device_type_t::SATA: return ahci_sata_read(device, lba, buffer);
        default: return false;
    }

    return false;
}

bool ahci_write(ahci_device_t* device) {
    if (!device)
        return false;

    // TODO @since 30/01/2026 -- 19:35

    return false;
}

bool is_ahci_device(const pci_device_t* device) {
    return device->class_info.prog_if == 0x1 &&
           device->class_info.class_code == 0x1 &&
           device->class_info.sub_class == 0x6;
}