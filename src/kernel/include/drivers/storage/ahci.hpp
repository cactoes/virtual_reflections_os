//==========================================
/// @file       ahci.hpp
/// @brief      s-ata, ahci, drive drivers
//==========================================

#pragma once

#ifndef __DRIVERS_STORAGE_AHCI_HPP__
#define __DRIVERS_STORAGE_AHCI_HPP__

#define AHCI_SSTS_DET_MASK      0x0000000F
#define AHCI_SSTS_SPD_MASK      0x000000F0
#define AHCI_SSTS_IPM_MASK      0x00000F00

#define AHCI_DET_NO_DEVICE          0x0
#define AHCI_DET_DEVICE_PRESENT     0x1
#define AHCI_DET_PHY_INITIALIZED    0x3

#define AHCI_SPD_GEN1   0x1
#define AHCI_SPD_GEN2   0x2
#define AHCI_SPD_GEN3   0x3

#define AHCI_IPM_ACTIVE     0x1
#define AHCI_IPM_PARTIAL    0x2
#define AHCI_IPM_SLUMBER    0x6

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

#define AHCI_PORT_INT_DHRS     (1 << 0)
#define AHCI_PORT_INT_PSS      (1 << 1)
#define AHCI_PORT_INT_DSS      (1 << 2)
#define AHCI_PORT_INT_SDBS     (1 << 3)
#define AHCI_PORT_INT_UFS      (1 << 4)
#define AHCI_PORT_INT_DPS      (1 << 5)
#define AHCI_PORT_INT_PCS      (1 << 6)
#define AHCI_PORT_INT_DMPS     (1 << 7)
#define AHCI_PORT_INT_PRCS     (1 << 22)
#define AHCI_PORT_INT_IPMS     (1 << 23)
#define AHCI_PORT_INT_OFS      (1 << 24)
#define AHCI_PORT_INT_INFS     (1 << 26)
#define AHCI_PORT_INT_IFS      (1 << 27)
#define AHCI_PORT_INT_HBDS     (1 << 28)
#define AHCI_PORT_INT_HBFS     (1 << 29)
#define AHCI_PORT_INT_TFES     (1 << 30)
#define AHCI_PORT_INT_CPDS     (1 << 31)

#define AHCI_PORT_SIG_NONE         0x00000000
#define AHCI_PORT_SIG_SATA         0x00000101
#define AHCI_PORT_SIG_ATAPI        0xEB140101
#define AHCI_PORT_SIG_SEMB         0xC33C0101
#define AHCI_PORT_SIG_PM           0x96690101

#define AHCI_PORT_CMD_ST        (1 << 0)
#define AHCI_PORT_CMD_SUD       (1 << 1)
#define AHCI_PORT_CMD_POD       (1 << 2)
#define AHCI_PORT_CMD_CLO       (1 << 3)
#define AHCI_PORT_CMD_FRE       (1 << 4)
#define AHCI_PORT_CMD_CCS_MASK  0x1F << 8
#define AHCI_PORT_CMD_FR        (1 << 14)
#define AHCI_PORT_CMD_CR        (1 << 15)
#define AHCI_PORT_CMD_CPS       (1 << 16)
#define AHCI_PORT_CMD_PMA       (1 << 17)
#define AHCI_PORT_CMD_HPCP      (1 << 18)
#define AHCI_PORT_CMD_MPSP      (1 << 19)
#define AHCI_PORT_CMD_CPD       (1 << 20)
#define AHCI_PORT_CMD_ESP       (1 << 21)
#define AHCI_PORT_CMD_FBSCP     (1 << 22)
#define AHCI_PORT_CMD_APSTE     (1 << 23)
#define AHCI_PORT_CMD_ATAPI     (1 << 24)
#define AHCI_PORT_CMD_DLAE      (1 << 25)
#define AHCI_PORT_CMD_ALPE      (1 << 26)
#define AHCI_PORT_CMD_ASP       (1 << 27)
#define AHCI_PORT_CMD_ICC_MASK  0xF << 28

#define AHCI_GHC_HR        (1 << 0)
#define AHCI_GHC_IE        (1 << 1)
#define AHCI_GHC_MRSM      (1 << 2)
#define AHCI_GHC_AE        (1 << 31)

#include "common.hpp"
#include "drivers/storage/storage.hpp"
#include "drivers/pcie.hpp"
#include "utils/vector.hpp"

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

    uint8_t rsv[116];
    uint8_t vendor[96];

    hba_port_t ports[32];
};

struct hba_cmd_header_t {
    uint8_t  cfl     : 5;
    uint8_t  a       : 1;
    uint8_t  w       : 1;
    uint8_t  p       : 1;
    uint8_t  r       : 1;
    uint8_t  b       : 1;
    uint8_t  c       : 1;
    uint8_t  rsv0    : 1;
    uint8_t  pmp     : 4;
    uint16_t prdtl;

    volatile uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} PACKED;

struct fis_reg_h2d_t {
    uint8_t fis_type;
    uint8_t pmport     : 4;
    uint8_t rsv0       : 3;
    uint8_t c          : 1;
    uint8_t command;
    uint8_t featurel;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;

    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;

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
        
        uint32_t dbc    : 22;
        uint32_t rsv1   : 9;
        uint32_t i      : 1;
    } prdt_entry[1];
} PACKED;

struct ahci_cmd_context_t {
    hba_cmd_header_t* cmdheader;
    hba_cmd_tbl_t* cmdtable;
    void* data_buffer;
    fis_reg_h2d_t* fis;
    uint8_t slot;
};

struct ahci_drive_t {
    char model[41];
    char serial[21];
    char firmware[9];

    uint64_t lba;
    uint64_t capacity;
    uint64_t logical_sector_size;
    uint64_t physical_sector_size;

    void* clb;
    hba_port_t* port;

    bool was_setup;
};

class ahci_storage_driver_t : public storage_driver_interface_t {
public:
    ahci_storage_driver_t(ahci_drive_t* drive);

    bool read(uint32_t lba, uint8_t* buffer, size_t size) override;
    bool write(uint32_t lba, uint8_t* buffer, size_t size) override;

private:
    ahci_drive_t* drive;
};

int ahci_init(const pci_device_t* pice_device, linked_list<ahci_drive_t>* device_list);

void* ahci_port_init(hba_port_t* port);
int ahci_find_command_slot(hba_port_t* port);

int ahci_sata_identify_device(ahci_drive_t* drive);
int ahci_sata_prepare_command(ahci_cmd_context_t* ctx, ahci_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t ata_command, bool write, uint8_t fis_device, uint8_t slot = 0);
int ahci_sata_read(ahci_drive_t* drive, uint64_t lba, uint16_t sector_count, uint8_t* buffer);
int ahci_sata_write(ahci_drive_t* drive, uint64_t lba, uint16_t sector_count, const uint8_t* buffer);

int ahci_atapi_identify_device(ahci_drive_t* drive);
int ahci_atapi_prepare_command(ahci_cmd_context_t* ctx, ahci_drive_t* drive, const uint8_t* atapi_packet, size_t packet_size, uint16_t data_length, bool write, uint8_t slot = 0);

#endif // __DRIVERS_STORAGE_AHCI_HPP__