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

#define ATA_CMD_IDENTIFY_DEVICE   0xEC
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_READ_SECTORS      0x20
#define ATA_CMD_WRITE_SECTORS     0x30

#define ATA_DEV_LBA               0x40
#define ATA_DEV_DEFAULT           0xA0
#define ATA_DEV_LBA_MASTER        (ATA_DEV_DEFAULT | ATA_DEV_LBA)

#define FIS_TYPE_REG_H2D        0x27
#define FIS_TYPE_REG_D2H        0x34
#define FIS_TYPE_DMA_ACT        0x39
#define FIS_TYPE_DMA_SETUP      0x41
#define FIS_TYPE_DATA           0x46
#define FIS_TYPE_BIST           0x58
#define FIS_TYPE_PIO_SETUP      0x5F
#define FIS_TYPE_DEV_BITS       0xA1

#define AHCI_PORT_INT_DHRS     (1 << 0)   // Device-to-Host Register FIS interrupt
#define AHCI_PORT_INT_PSS      (1 << 1)   // PIO Setup FIS interrupt
#define AHCI_PORT_INT_DSS      (1 << 2)   // DMA Setup FIS interrupt
#define AHCI_PORT_INT_SDBS     (1 << 3)   // Set Device Bits FIS interrupt
#define AHCI_PORT_INT_UFS      (1 << 4)   // Unknown FIS interrupt
#define AHCI_PORT_INT_DPS      (1 << 5)   // Descriptor Processed interrupt
#define AHCI_PORT_INT_PCS      (1 << 6)   // Port Connect Change Status
#define AHCI_PORT_INT_DMPS     (1 << 7)   // Device Mechanical Presence Status
#define AHCI_PORT_INT_PRCS     (1 << 22)  // PhyRdy Change Status
#define AHCI_PORT_INT_IPMS     (1 << 23)  // Incorrect Port Multiplier Status
#define AHCI_PORT_INT_OFS      (1 << 24)  // Overflow Status
#define AHCI_PORT_INT_INFS     (1 << 26)  // Interface Non-Fatal Error
#define AHCI_PORT_INT_IFS      (1 << 27)  // Interface Fatal Error
#define AHCI_PORT_INT_HBDS     (1 << 28)  // Host Bus Data Error
#define AHCI_PORT_INT_HBFS     (1 << 29)  // Host Bus Fatal Error
#define AHCI_PORT_INT_TFES     (1 << 30)  // Task File Error Status
#define AHCI_PORT_INT_CPDS     (1 << 31)  // Cold Port Detect Status

#define AHCI_PORT_CMD_ST        (1 << 0)   // Start
#define AHCI_PORT_CMD_SUD       (1 << 1)   // Spin-Up Device
#define AHCI_PORT_CMD_POD       (1 << 2)   // Power On Device
#define AHCI_PORT_CMD_CLO       (1 << 3)   // Command List Override
#define AHCI_PORT_CMD_FRE       (1 << 4)   // FIS Receive Enable
#define AHCI_PORT_CMD_CCS_MASK  0x1F << 8  // Current Command Slot (bits 8–12)
#define AHCI_PORT_CMD_FR        (1 << 14)  // FIS Receive Running
#define AHCI_PORT_CMD_CR        (1 << 15)  // Command List Running
#define AHCI_PORT_CMD_CPS       (1 << 16)  // Cold Presence State
#define AHCI_PORT_CMD_PMA       (1 << 17)  // Port Multiplier Attached
#define AHCI_PORT_CMD_HPCP      (1 << 18)  // Hot Plug Capable Port
#define AHCI_PORT_CMD_MPSP      (1 << 19)  // Mechanical Presence Switch Attached
#define AHCI_PORT_CMD_CPD       (1 << 20)  // Cold Presence Detection
#define AHCI_PORT_CMD_ESP       (1 << 21)  // External SATA Port
#define AHCI_PORT_CMD_FBSCP     (1 << 22)  // FIS-based Switching Capable Port
#define AHCI_PORT_CMD_APSTE     (1 << 23)  // Aggressive Link Power Management Enable
#define AHCI_PORT_CMD_ATAPI     (1 << 24)  // Device is ATAPI
#define AHCI_PORT_CMD_DLAE      (1 << 25)  // Drive LED on ATAPI Enable
#define AHCI_PORT_CMD_ALPE      (1 << 26)  // Aggressive Link Power Management Enable
#define AHCI_PORT_CMD_ASP       (1 << 27)  // Aggressive Slumber/Partial
#define AHCI_PORT_CMD_ICC_MASK  0xF << 28  // Interface Communication Control (bits 28–31)

#define AHCI_PORT_SIG_NONE         0x00000000  // No device present / empty port
#define AHCI_PORT_SIG_SATA         0x00000101  // SATA drive (ATA)
#define AHCI_PORT_SIG_ATAPI        0xEB140101  // ATAPI device (e.g. CD/DVD)
#define AHCI_PORT_SIG_SEMB         0xC33C0101  // Enclosure management bridge
#define AHCI_PORT_SIG_PM           0x96690101  // Port multiplier

#define AHCI_SSTS_DET_MASK      0x0000000F  // Device Detection
#define AHCI_SSTS_SPD_MASK      0x000000F0  // Current Interface Speed
#define AHCI_SSTS_IPM_MASK      0x00000F00  // Interface Power Management

#define AHCI_DET_NO_DEVICE          0x0  // No device detected
#define AHCI_DET_DEVICE_PRESENT     0x1  // Device present, but PHY not initialized
#define AHCI_DET_PHY_INITIALIZED    0x3  // Device present, PHY communication established

#define AHCI_SPD_GEN1   0x1  // 1.5 Gbps
#define AHCI_SPD_GEN2   0x2  // 3.0 Gbps
#define AHCI_SPD_GEN3   0x3  // 6.0 Gbps

#define AHCI_IPM_ACTIVE     0x1  // Interface active
#define AHCI_IPM_PARTIAL    0x2  // Interface in partial power state
#define AHCI_IPM_SLUMBER    0x6  // Interface in slumber state

#define AHCI_GHC_HR        (1 << 0)   // HBA Reset
#define AHCI_GHC_IE        (1 << 1)   // Interrupt Enable
#define AHCI_GHC_MRSM      (1 << 2)   // MSI Revert to Single Message
#define AHCI_GHC_AE        (1U << 31) // AHCI Enable

#include "common.hpp"
#include "drivers/pci_driver.hpp"
#include "vector.hpp"
#include "file_systems/vfs.hpp"

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

struct ahci_cmd_context_t {
    hba_cmd_header_t* cmdheader;
    hba_cmd_tbl_t* cmdtable;
    void* data_buffer;
    fis_reg_h2d_t* fis;
    uint8_t slot;
};

int ahci_init(void* pml4, pci_device_info_t* ahci_pci_device, vector<ahci_sata_drive_t>* sata_drives);
void* ahci_port_init(hba_port_t* port);
void* ahci_atapi_port_init(hba_port_t* port);

int ahci_identify_device(ahci_sata_drive_t* drive);
int ahci_atapi_identify_device(ahci_sata_drive_t* drive);
int ahci_read(ahci_sata_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t* buffer);
int ahci_write(ahci_sata_drive_t* drive, uint64_t lba, uint16_t sector_count, const void* buffer);

int ahci_prepare_command(ahci_cmd_context_t* ctx, ahci_sata_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t ata_command, bool write, uint8_t fis_device, uint8_t slot = 0);
int ahci_atapi_prepare_command(ahci_cmd_context_t* ctx, ahci_sata_drive_t* drive, const uint8_t* atapi_packet, size_t packet_size, uint16_t data_length, bool write, uint8_t slot = 0);
int ahci_find_command_slot(hba_port_t* port);

inline int ahci_dev_read(drive_t* drive, uint32_t lba, void* buffer, size_t* size) {
    if (*size != 512)
        return 1;

    return ahci_read((ahci_sata_drive_t*)drive->device, lba, 1, (uint8_t*)buffer);
}

inline int ahci_dev_write(drive_t* drive, uint32_t lba, void* buffer, size_t* size) {
    return 1;
}

#endif // __AHCI_DRIVER_HPP__