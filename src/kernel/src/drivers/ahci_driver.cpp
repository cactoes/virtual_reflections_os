#include "drivers/ahci_driver.hpp"
#include "debug.hpp"
#include "memory.hpp"

dma_heap_t g_dma_heap {};

volatile hba_mem_t* g_hba_mem = nullptr;

int ahci_init(void* pml4, pci_device_info_t* ahci_pci_device, vector<ahci_sata_drive_t>* sata_drives) {
    if (dma_heap_init(pml4, &g_dma_heap, (void*)AHCI_DMA_HEAP_ADDR) != 0)
        return 1;

    uint64_t bar_addr_physical = ahci_pci_device->bar5_address & ~0xF;
    uint64_t bar_page_addr_physical = mem_align_down(bar_addr_physical, PAGE_SIZE_LARGE);
    uint64_t bar_addr_offset = bar_addr_physical - bar_page_addr_physical;

    if (!vmem_map_2mb_page(pml4, (void*)AHCI_HBA_ADDR, (void*)bar_page_addr_physical))
        return 2;
    
    g_hba_mem = (volatile hba_mem_t*)((uint8_t*)AHCI_HBA_ADDR + bar_addr_offset);

    uint32_t version = g_hba_mem->vs;
    uint32_t ports_implemented = g_hba_mem->pi;

    // g_hba_mem->ghc |= AHCI_GHC_IE;  // Interrupt enable -> irq11
    g_hba_mem->ghc |= AHCI_GHC_AE; // AHCI Enable

    for (int i = 0; i < 32; i++) {
        if (ports_implemented & (1 << i)) {
            volatile hba_port_t* port = &g_hba_mem->ports[i];

            uint32_t ssts = port->ssts;

            uint8_t det = ssts & AHCI_SSTS_DET_MASK;
            // uint8_t spd = (ssts & AHCI_SSTS_SPD_MASK) >> 4;
            uint8_t ipm = (ssts & AHCI_SSTS_IPM_MASK) >> 8;

            if (det != AHCI_DET_PHY_INITIALIZED || ipm != AHCI_IPM_ACTIVE)
                continue;

            switch (port->sig) {
                case AHCI_PORT_SIG_SATA: {
                    ahci_sata_drive_t drive {};
                    drive.port = (hba_port_t*)port;
                    
                    if (!(drive.clb = ahci_port_init((hba_port_t*)port))) {
                        debug_print("SATA device failed to init port");
                        break;
                    }

                    if (ahci_identify_device(&drive) != 0) {
                        debug_print("SATA device failed to identify port");
                        break;
                    }
                    
                    sata_drives->insert_back(drive);
                    break;
                }
                default:
                    break;
            }
        }
    }

    return 0;
}

void* ahci_port_init(hba_port_t* port) {
    port->cmd &= ~0x01;

    while (port->cmd & AHCI_PORT_CMD_CR);

    port->cmd &= ~AHCI_PORT_CMD_FRE;

    auto clb = dma_heap_alloc(&g_dma_heap);
    if (!clb)
        return nullptr;

    memset(clb, 0, sizeof(dma_memory_region_t::block_t));
    port->clb = dma_get_physical_lower(&g_dma_heap, clb);
    port->clbu = dma_get_physical_upper(&g_dma_heap, clb);

    auto rfis = dma_heap_alloc(&g_dma_heap);
    if (!rfis) {
        dma_heap_free(&g_dma_heap, clb);
        return nullptr;
    }

    memset(rfis, 0, sizeof(dma_memory_region_t::block_t));
    port->fb = dma_get_physical_lower(&g_dma_heap, rfis);
    port->fbu = dma_get_physical_upper(&g_dma_heap, rfis);

    // disable interrupts
    port->ie = 0;
    // enable all interrupts
    // port->ie = 0xFFFFFFFF; -> irq11

    port->cmd |= AHCI_PORT_CMD_FRE;
    port->cmd |= AHCI_PORT_CMD_ST;

    return clb;
}

void decode_string(const uint16_t* src, int word_count, char* dest, int max_len) {
    int pos = 0;
    for (int i = 0; i < word_count && pos + 1 < max_len; ++i) {
        dest[pos++] = (char)(src[i] >> 8);
        dest[pos++] = (char)(src[i] & 0xFF);
    }
    dest[pos] = '\0';
}

int ahci_identify_device(ahci_sata_drive_t* drive) {
    int slot = ahci_find_command_slot(drive->port);
    if (slot < 0)
        return 1;

    ahci_cmd_context_t ctx {};
    if (ahci_prepare_command(&ctx, drive, 0, 1, ATA_CMD_IDENTIFY_DEVICE, false, ATA_DEV_DEFAULT, slot) != 0)
        return 2;

    drive->port->is = (uint32_t)-1;
    drive->port->ci = 1 << slot;

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

    drive->logical_sector_size  = (uint32_t)data[117] | ((uint32_t)data[118] << 16);
    if (drive->logical_sector_size  == 0) drive->logical_sector_size  = 512;

    drive->physical_sector_size = drive->logical_sector_size ;
    uint16_t word106 = data[106];
    if (word106 & (1 << 13)) {
        uint8_t log2_multiple = word106 & 0x1F;
        drive->physical_sector_size = drive->logical_sector_size  << log2_multiple;
    }

    drive->capacity = drive->lba * (uint64_t)drive->logical_sector_size ;

    dma_heap_free(&g_dma_heap, (dma_memory_region_t::block_t*)ctx.cmdtable);
    dma_heap_free(&g_dma_heap, ctx.data_buffer);

    return 0;
}

int ahci_read(ahci_sata_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t* buffer) {
    int slot = ahci_find_command_slot(drive->port);
    if (slot < 0)
        return 1;

    ahci_cmd_context_t ctx {};
    ahci_prepare_command(&ctx, drive, lba, sector_count, ATA_CMD_READ_DMA_EXT, false, ATA_DEV_LBA, slot);

    drive->port->is = (uint32_t)-1;
    drive->port->ci = 1 << slot;

    // Wait for completion
    while (drive->port->ci & (1 << slot)) {
        if (drive->port->is & AHCI_PORT_INT_TFES) {
            // debug_print("AHCI read: Task file error");
            return 2;
        }
    }

    if (drive->port->is & AHCI_PORT_INT_TFES) {
		// debug_print("AHCI read failed: Read disk error\n");
		return 3;
	}

    memcpy(buffer, ctx.data_buffer, drive->logical_sector_size);

    dma_heap_free(&g_dma_heap, (dma_memory_region_t::block_t*)ctx.cmdtable);
    dma_heap_free(&g_dma_heap, ctx.data_buffer);
    return 0;
}

int ahci_write(ahci_sata_drive_t* drive, uint64_t lba, uint16_t sector_count, const void* buffer) {
    int slot = ahci_find_command_slot(drive->port);
    if (slot < 0)
        return 1;

    ahci_cmd_context_t ctx {};
    ahci_prepare_command(&ctx, drive, lba, sector_count, ATA_CMD_WRITE_DMA_EXT, true, ATA_DEV_LBA, slot);

    memcpy(ctx.data_buffer, buffer, sector_count * drive->logical_sector_size);

    drive->port->is = (uint32_t)-1;
    drive->port->ci |= (1 << slot);

    while (drive->port->ci & (1 << slot)) {
        if (drive->port->is & AHCI_PORT_INT_TFES) {
            // debug_print("AHCI write: Task file error\n");
            return 2;
        }
    }

    if (drive->port->is & AHCI_PORT_INT_TFES) {
        // debug_print("AHCI write failed: Disk error\n");
		return 3;
    }

    dma_heap_free(&g_dma_heap, (dma_memory_region_t::block_t*)ctx.cmdtable);
    dma_heap_free(&g_dma_heap, ctx.data_buffer);

    return 0;
}

int ahci_prepare_command(ahci_cmd_context_t* ctx, ahci_sata_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t ata_command, bool write, uint8_t fis_device, uint8_t slot) {
    ctx->slot = slot;

    ctx->cmdheader = (hba_cmd_header_t*)drive->clb;
    memset(&ctx->cmdheader[slot], 0, sizeof(hba_cmd_header_t));

    ctx->cmdheader[slot].cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
    ctx->cmdheader[slot].w = write ? 1 : 0;
    ctx->cmdheader[slot].prdtl = 1;

    ctx->cmdtable = (hba_cmd_tbl_t*)dma_heap_alloc(&g_dma_heap);
    if (!ctx->cmdtable)
        return 1;

    memset(ctx->cmdtable, 0, sizeof(dma_memory_region_t::block_t));

    ctx->cmdheader[slot].ctba = dma_get_physical_lower(&g_dma_heap, (dma_memory_region_t::block_t*)ctx->cmdtable);
    ctx->cmdheader[slot].ctbau = dma_get_physical_upper(&g_dma_heap, (dma_memory_region_t::block_t*)ctx->cmdtable);

    ctx->data_buffer = dma_heap_alloc(&g_dma_heap);
    if (!ctx->data_buffer) {
        dma_heap_free(&g_dma_heap, (dma_memory_region_t::block_t*)ctx->cmdtable);
        return 2;
    }

    memset(ctx->data_buffer, 0, sizeof(dma_memory_region_t::block_t));

    ctx->cmdtable->prdt_entry[0].dba = dma_get_physical_lower(&g_dma_heap, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbau = dma_get_physical_upper(&g_dma_heap, ctx->data_buffer);
    ctx->cmdtable->prdt_entry[0].dbc = (drive->logical_sector_size * sector_count) - 1;
    ctx->cmdtable->prdt_entry[0].i = 0; // disable interrupts

    ctx->fis = (fis_reg_h2d_t*)(&ctx->cmdtable->cfis);
    memset(ctx->fis, 0, sizeof(fis_reg_h2d_t));
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

int ahci_find_command_slot(hba_port_t* port) {
    uint32_t slots = port->sact | port->ci;

    for (int i = 0; i < 32; ++i) {
        if ((slots & (1 << i)) == 0)
            return i;
    }

    return -1;
}