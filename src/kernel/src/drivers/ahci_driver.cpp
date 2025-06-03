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

    for (int i = 0; i < 32; i++) {
        if (ports_implemented & (1 << i)) {
            volatile hba_port_t* port = &g_hba_mem->ports[i];
            // debug_print("port[%i] SATA status: 0x%uh; signature: 0x%uh \n", i, port->ssts, port->sig);

            // uint8_t ipm = (ssts >> 8) & 0x0F;
            // uint8_t det = ssts & 0x0F;
            // if (det != HBA_PORT_DET_PRESENT)
            //     continue;
            // if (ipm != HBA_PORT_IPM_ACTIVE)
            //     continue;

            if (port->sig == 0x101) { // sig: SATA
                ahci_sata_drive_t drive {};
                drive.port = (hba_port_t*)port;

                drive.clb = ahci_port_init((hba_port_t*)port);
                ahci_identify_device(pml4, &drive);
                
                sata_drives->insert_back(drive);
            }
        }
    }

    return 0;
}

void* ahci_port_init(hba_port_t* port) {
    port->cmd &= ~0x01;

    while (port->cmd & (1 << 15));

    port->cmd &= ~(1 << 4);

    auto clb = dma_heap_alloc(&g_dma_heap);
    auto rfis = dma_heap_alloc(&g_dma_heap);\

    memset(clb, 0, 4096);
    memset(rfis, 0, 4096);

    port->clb = dma_get_physical_lower(&g_dma_heap, clb);
    port->clbu = dma_get_physical_upper(&g_dma_heap, clb);
    port->fb = dma_get_physical_lower(&g_dma_heap, rfis);
    port->fbu = dma_get_physical_upper(&g_dma_heap, rfis);

    // disable interrupts
    port->ie = 0;

    port->cmd |= (1 << 4);
    port->cmd |= (1 << 0);

    return clb;
}

// Convert 16-bit word string (big-endian chars) to C string
void decode_string(const uint16_t* src, int word_count, char* dest, int max_len) {
    int pos = 0;
    for (int i = 0; i < word_count && pos + 1 < max_len; ++i) {
        dest[pos++] = (char)(src[i] >> 8);
        dest[pos++] = (char)(src[i] & 0xFF);
    }
    dest[pos] = '\0';
    // trim(dest);
}

void ahci_identify_device(void* pml4, ahci_sata_drive_t* ahci_drive_data) {
    // uint64_t clb_p = (uint64_t)port->clb | ((uint64_t)port->clbu << 32);
    // void* clb = vmem_physical_to_virtual(pml4, (void*)AHCI_DMA_HEAP_ADDR, (void*)(AHCI_DMA_HEAP_ADDR + PAGE_SIZE_LARGE), (void*)clb_p);

    hba_cmd_header_t* cmdheader = (hba_cmd_header_t*)ahci_drive_data->clb;
    cmdheader[0].cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);  // Command FIS length in DWORDs
    cmdheader[0].w = 0;     // READ = 0, WRITE = 1
    cmdheader[0].prdtl = 1; // 1 PRDT

    // Allocate Command Table (256 + PRDT entries)
    auto cmdtbl = dma_heap_alloc(&g_dma_heap);
    memset(cmdtbl, 0, 4096);
    cmdheader[0].ctba = dma_get_physical_lower(&g_dma_heap, cmdtbl);
    cmdheader[0].ctbau = dma_get_physical_upper(&g_dma_heap, cmdtbl);

    // Fill PRDT to receive 512 bytes
    hba_cmd_tbl_t* tbl = (hba_cmd_tbl_t*)cmdtbl;
    auto identify_buf = dma_heap_alloc(&g_dma_heap);
    tbl->prdt_entry[0].dba = dma_get_physical_lower(&g_dma_heap, identify_buf);
    tbl->prdt_entry[0].dbau = dma_get_physical_upper(&g_dma_heap, identify_buf);
    tbl->prdt_entry[0].dbc = 512 - 1;
    tbl->prdt_entry[0].i = 0;  // Interrupt on completion

    fis_reg_h2d_t* fis = (fis_reg_h2d_t*)(&tbl->cfis);
    fis->fis_type = 0x27; // FIS_TYPE_REG_H2D;
    fis->c = 1;  // command
    fis->command = 0xEC;  // IDENTIFY DEVICE
    fis->device = 0;      // Drive 0

    ahci_drive_data->port->ci = 1 << 0;  // Command slot 0

    // Wait for completion
    while (ahci_drive_data->port->ci & (1 << 0)) {
        // optional: timeout
    }

    const uint16_t* data = (const uint16_t*)identify_buf;

    char model[41], serial[21], firmware[9];

    decode_string(&data[27], 20, model, sizeof(model));
    decode_string(&data[10], 10, serial, sizeof(serial));
    decode_string(&data[23], 4, firmware, sizeof(firmware));

    // LBA28 fallback
    uint64_t total_lba = (uint32_t)data[60] | ((uint32_t)data[61] << 16);

    // Check for 48-bit LBA
    if (data[83] & (1 << 10)) {
        total_lba = ((uint64_t)data[100]) |
                    ((uint64_t)data[101] << 16) |
                    ((uint64_t)data[102] << 32) |
                    ((uint64_t)data[103] << 48);
    }

    // Logical sector size
    uint32_t logical_sector_size = (uint32_t)data[117] | ((uint32_t)data[118] << 16);
    if (logical_sector_size == 0) logical_sector_size = 512;

    // Physical sector size
    uint32_t physical_sector_size = logical_sector_size;
    uint16_t word106 = data[106];
    if (word106 & (1 << 13)) {
        uint8_t log2_multiple = word106 & 0x1F;
        physical_sector_size = logical_sector_size << log2_multiple;
    }

    uint64_t capacity_bytes = total_lba * (uint64_t)logical_sector_size;

    memcpy(ahci_drive_data->model, model, 41);
    memcpy(ahci_drive_data->serial, serial, 21);
    memcpy(ahci_drive_data->firmware, firmware, 9);

    ahci_drive_data->lba = total_lba;
    ahci_drive_data->capacity = capacity_bytes;
    ahci_drive_data->logical_sector_size = logical_sector_size;
    ahci_drive_data->physical_sector_size = physical_sector_size;

    dma_heap_free(&g_dma_heap, cmdtbl);
    dma_heap_free(&g_dma_heap, identify_buf);

    // Print results
    // debug_print("Model Number           : %s\n", model);
    // debug_print("Serial Number          : %s\n", serial);
    // debug_print("Firmware Revision      : %s\n", firmware);
    // debug_print("Total LBA Sectors      : %ul\n", total_lba);
    // debug_print("Drive Capacity         : %ul bytes\n", capacity_bytes);
    // debug_print("Logical Sector Size    : %u bytes\n", logical_sector_size);
    // debug_print("Physical Sector Size   : %u bytes\n", physical_sector_size);
}

void ahci_read(hba_port_t* port) {

}