//==========================================
/// @file       ahci.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __AHCI_HPP__
#define __AHCI_HPP__

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
#include "drivers/pcie.hpp"
#include "std/array.hpp"
#include "memory/heap.hpp"
#include "utils/mutex.hpp"

struct hba_port_t {
    volatile u32 clb;
    volatile u32 clbu;
    volatile u32 fb;
    volatile u32 fbu;
    volatile u32 is;
    volatile u32 ie;
    volatile u32 cmd;
    volatile u32 rsv0;
    volatile u32 tfd;
    volatile u32 sig;
    volatile u32 ssts;
    volatile u32 sctl;
    volatile u32 serr;
    volatile u32 sact;
    volatile u32 ci;
    volatile u32 sntf;
    volatile u32 fbs;
    volatile u32 rsv1[11];
    volatile u32 vendor[4];
};

struct hba_mem_t {
    volatile u32 cap;
    volatile u32 ghc;
    volatile u32 is;
    volatile u32 pi;
    volatile u32 vs;
    volatile u32 ccc_ctl;
    volatile u32 ccc_pts;
    volatile u32 em_loc;
    volatile u32 em_ctl;
    volatile u32 cap2;
    volatile u32 bohc;

    volatile u8 rsv[116];
    volatile u8 vendor[96];

    volatile hba_port_t ports[32];
};

struct hba_cmd_header_t {
    u8  cfl     : 5;
    u8  a       : 1;
    u8  w       : 1;
    u8  p       : 1;
    u8  r       : 1;
    u8  b       : 1;
    u8  c       : 1;
    u8  rsv0    : 1;
    u8  pmp     : 4;
    u16 prdtl;

    volatile u32 prdbc;
    u32 ctba;
    u32 ctbau;
    u32 rsv1[4];
} PACKED;

struct fis_reg_h2d_t {
    u8 fis_type;
    u8 pmport     : 4;
    u8 rsv0       : 3;
    u8 c          : 1;
    u8 command;
    u8 featurel;

    u8 lba0;
    u8 lba1;
    u8 lba2;
    u8 device;

    u8 lba3;
    u8 lba4;
    u8 lba5;
    u8 featureh;

    u8 countl;
    u8 counth;
    u8 icc;
    u8 control;

    u8 rsv1[4];
} PACKED;

struct hba_cmd_tbl_t {
    u8 cfis[64];
    u8 acmd[16];
    u8 rsv[48];

    struct hba_prdt_entry_t {
        u32 dba;
        u32 dbau;
        u32 rsv0;
        
        u32 dbc    : 22;
        u32 rsv1   : 9;
        u32 i      : 1;
    } prdt_entry[1];
} PACKED;

struct ahci_cmd_context_t {
    hba_cmd_header_t* cmdheader;
    hba_cmd_tbl_t* cmdtable;
    void* data_buffer;
    fis_reg_h2d_t* fis;
    u8 slot;
};

struct ahci_driver_ctx_t {
    heap_t* dma;
};

enum class ahci_device_type_t {
    UNKNOWN = 0,
    SATA
};

struct ahci_device_t {
    void* clb;
    volatile hba_port_t* port;

    ahci_device_type_t type;

    u64 lba_count;
    u64 capacity;
    u64 logical_sector_size;
    u64 physical_sector_size;

    ahci_driver_ctx_t* ahci_driver_ctx;

    mutex_t mutex;

    struct {
        char model[41];
        char serial[21];
        char firmware[9];
    } meta;
};

bool ahci_init(const pci_device_t* device, ahci_driver_ctx_t* ahci_driver_ctx, std::dynamic_array<ahci_device_t>* device_list);
bool ahci_device_init(ahci_device_t* device);
bool ahci_read(ahci_device_t* device, u64 lba, u8* buffer, size_t size);
bool ahci_write(ahci_device_t* device);
bool is_ahci_device(const pci_device_t* device);

#endif // __AHCI_HPP__