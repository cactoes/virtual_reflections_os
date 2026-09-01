//==========================================
/// @file       nvme.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __NVME_HPP__
#define __NVME_HPP__

#define NVME_CAP_MQES(cap)   ((u16)((cap) & 0xFFFF))
#define NVME_CAP_TO(cap)     ((u8)(((cap) >> 24) & 0xFF))
#define NVME_CAP_DSTRD(cap)  ((u8)(((cap) >> 32) & 0xF))
#define NVME_CAP_CSS(cap)    ((u8)(((cap) >> 37) & 0xFF))
#define NVME_CAP_MPSMIN(cap) ((u8)(((cap) >> 48) & 0xF))

#define NVME_CC_EN        (1u << 0)
#define NVME_CC_CSS_NVM   (0u << 1)
#define NVME_CC_MPS(n)     ((u32)(n) << 7)
#define NVME_CC_IOSQES(n)  ((u32)(n) << 16)
#define NVME_CC_IOCQES(n)  ((u32)(n) << 20)

#define NVME_CSTS_RDY (1u << 0)
#define NVME_CSTS_CFS (1u << 1)

#define NVME_OPC_DELETE_SQ      0x00
#define NVME_OPC_CREATE_SQ      0x01
#define NVME_OPC_DELETE_CQ      0x04
#define NVME_OPC_CREATE_CQ      0x05
#define NVME_OPC_IDENTIFY       0x06
#define NVME_OPC_WRITE          0x01
#define NVME_OPC_READ           0x02

#define NVME_CNS_IDENTIFY_NS         0x00
#define NVME_CNS_IDENTIFY_CTRL       0x01
#define NVME_CNS_ACTIVE_NS_LIST      0x02

#include "common.hpp"
#include "drivers/pcie.hpp"
#include "utils/mutex.hpp"
#include "storage/disk_manager.hpp"

struct nvme_regs_t {
    u64 cap;
    u32 vs;
    u32 intms;
    u32 intmc;
    u32 cc;
    u32 rsvd0;
    u32 csts;
    u32 nssr;
    u32 aqa;
    u64 asq;
    u64 acq;
};

struct nvme_command_t {
    u32 cdw0;
    u32 nsid;
    u64 rsvd2;
    u64 mptr;
    u64 prp1;
    u64 prp2;
    u32 cdw10;
    u32 cdw11;
    u32 cdw12;
    u32 cdw13;
    u32 cdw14;
    u32 cdw15;
};

struct nvme_completion_t {
    u32 dw0;
    u32 dw1;
    u16 sq_head;
    u16 sq_id;
    u16 cid;
    u16 status;

    nvme_completion_t() = default;

    nvme_completion_t(const volatile nvme_completion_t& src)
        : dw0(src.dw0), dw1(src.dw1), sq_head(src.sq_head),
          sq_id(src.sq_id), cid(src.cid), status(src.status) {}
};

struct nvme_queue_t {
    nvme_command_t* sq;
    nvme_completion_t* cq;
    u64 sq_phys;
    u64 cq_phys;

    volatile u32* sq_doorbell;
    volatile u32* cq_doorbell;

    u16 depth;
    u16 sq_tail;
    u16 cq_head;
    u8 cq_phase;
    u16 next_cid;
};

struct nvme_driver_ctx_t {
    heap_t* dma;
    volatile nvme_regs_t* regs;
    u32 doorbell_stride;
    nvme_queue_t admin_q;
    nvme_queue_t io_q;
    mutex_t submit_mutex;

    struct {
        char model[41];
        char serial[21];
        char firmware[9];
    } meta;
};

struct nvme_device_t {
    nvme_driver_ctx_t* nvme_driver_ctx;

    u32 nsid;
    u64 block_count;
    u32 block_size;
};

bool nvme_init(const pci_device_t* device, nvme_driver_ctx_t* ctx, std::dynamic_array<nvme_device_t>* device_list);
bool is_nvme_device(const pci_device_t* device);
const disk_interface_t* get_nvme_disk_interface();

#endif // __NVME_HPP__