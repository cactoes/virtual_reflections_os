//==========================================
/// @file       ahci_driver.hpp
/// @brief      s-ata, ahci, drive drivers
//==========================================

#pragma once

#ifndef __AHCI_DRIVER_HPP__
#define __AHCI_DRIVER_HPP__

#define HBA_PORT_IPM_ACTIVE     1
#define HBA_PORT_DET_PRESENT    3

#define AHCI_DMA_HEAP_ADDR      0x3FC00000
#define AHCI_HBA_ADDR           (AHCI_DMA_HEAP_ADDR - PAGE_SIZE_LARGE)

#include "common.hpp"
#include "drivers/pci_driver.hpp"
#include "vector.hpp"

struct hba_port_t {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
};

struct hba_mem_t {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;

    uint8_t rsv[0xA0-0x2C];
    uint8_t vendor[0x100-0xA0];
    hba_port_t ports[32];
};

struct hba_cmd_header_t {
    // DWORD
    uint8_t  cfl     : 5;  // Command FIS length in DWORDS (2 DWORDs = 8 bytes min)
    uint8_t  a       : 1;  // ATAPI
    uint8_t  w       : 1;  // Write (1 = write, 0 = read)
    uint8_t  p       : 1;  // Prefetchable
    uint8_t  r       : 1;  // Reset
    uint8_t  b       : 1;  // BIST
    uint8_t  c       : 1;  // Clear Busy upon R_OK
    uint8_t  rsv0    : 1;  // Reserved
    uint8_t  pmp     : 4;  // Port multiplier port
    uint16_t prdtl;

    volatile uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} PACKED;

struct fis_reg_h2d_t {
    // DWORD
    uint8_t fis_type;
    uint8_t pmport     : 4;
    uint8_t rsv0       : 3;
    uint8_t c          : 1;
    uint8_t command;
    uint8_t featurel;

    // DWORD 1
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    // DWORD 2
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;

    // DWORD 3
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;

    // DWORD 4
    uint8_t rsv1[4];
} PACKED;

struct hba_cmd_tbl_t {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];

    struct hba_prdt_entry_t {
        uint32_t dba;
        uint32_t dbau;
        uint32_t rsv0;
        
        // DWORD
        uint32_t dbc    : 22;
        uint32_t rsv1   : 9;
        uint32_t i      : 1;
    } prdt_entry[1];
} PACKED;

struct ahci_sata_drive_t {
    char model[41];
    char serial[21];
    char firmware[9];

    uint64_t lba;
    uint64_t capacity;
    uint64_t logical_sector_size;
    uint64_t physical_sector_size;

    void* clb;
    hba_port_t* port;
};

int ahci_init(void* pml4, pci_device_info_t* ahci_pci_device, vector<ahci_sata_drive_t>* sata_drives);
void* ahci_port_init(hba_port_t* port);
void ahci_identify_device(void* pml4, ahci_sata_drive_t* ahci_drive_data);
void ahci_read(hba_port_t* port);

#endif // __AHCI_DRIVER_HPP__