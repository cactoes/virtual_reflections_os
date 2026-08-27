#include "drivers/storage/nvme.hpp"
#include "memory/heap.hpp"
#include "memory/vmem.hpp"
#include "io.hpp"
#include "arch/amd64/cpu.hpp"

bool nvme_poll_until(volatile u32* reg, u32 mask, u32 want, u32 max_iters = 5000000) {
    for (u32 i = 0; i < max_iters; i++) {
        if ((*reg & mask) == want)
            return true;

        amd64_pause();
    }

    return false;
}

void* nvme_dma_alloc(heap_t* heap, u64 size, u64* out_phys, u64 alignment = 8192) {
    if (size < alignment)
        size = alignment;

    void* virt = dma_heap_alloc(heap, size, alignment);
    if (!virt)
        return nullptr;

    *out_phys = dma_get_physical(heap, virt);
    return virt;
}

bool nvme_queue_alloc(nvme_driver_ctx_t* ctx, nvme_queue_t* q, u16 qid, u16 depth) {
    q->depth = depth;
    q->sq_tail = 0;
    q->cq_head = 0;
    q->cq_phase = 1;
    q->next_cid = 0;

    const u64 sq_bytes = (u64)depth * sizeof(nvme_command_t);
    const u64 cq_bytes = (u64)depth * sizeof(nvme_completion_t);

    q->sq = (nvme_command_t*)nvme_dma_alloc(ctx->dma, sq_bytes, &q->sq_phys);
    if (!q->sq)
        return false;

    q->cq = (nvme_completion_t*)nvme_dma_alloc(ctx->dma, cq_bytes, &q->cq_phys);
    if (!q->cq)
        return false;

    for (u64 i = 0; i < cq_bytes; i++)
        ((volatile u8*)q->cq)[i] = 0;

    for (u64 i = 0; i < sq_bytes; i++)
        ((volatile u8*)q->sq)[i] = 0;

    const u8* base = (const u8*)ctx->regs + 0x1000;
    q->sq_doorbell = (volatile u32*)(base + (2 * qid) * ctx->doorbell_stride);
    q->cq_doorbell = (volatile u32*)(base + (2 * qid + 1) * ctx->doorbell_stride);

    return true;
}

bool nvme_submit_and_wait(nvme_driver_ctx_t* ctx, nvme_queue_t* q, nvme_command_t cmd, nvme_completion_t* out_completion = nullptr) {
    const u16 cid = q->next_cid++;
    cmd.cdw0 = (cmd.cdw0 & 0x0000FFFF) | ((u32)cid << 16);
    
    // Write the command to the submission queue
    q->sq[q->sq_tail] = cmd;
    u16 old_tail = q->sq_tail;
    q->sq_tail = (q->sq_tail + 1) % q->depth;
    
    amd64_mem_barier();
    *q->sq_doorbell = q->sq_tail;

    for (u32 spins = 0; spins < 5000000; spins++) {
        amd64_mem_barier();
        nvme_completion_t entry = q->cq[q->cq_head];
        if ((entry.status & 1) == q->cq_phase) {
            if (out_completion)
                *out_completion = entry;

            q->cq_head = (q->cq_head + 1) % q->depth;
            if (q->cq_head == 0)
                q->cq_phase ^= 1;

            *q->cq_doorbell = q->cq_head;

            const u16 sc = (entry.status >> 1) & 0x7FFF;
            if (sc != 0)
                return false;
            return true;
        }
        amd64_pause();
    }

    return false;
}

bool nvme_identify(nvme_driver_ctx_t* ctx, u32 nsid, u8 cns, void* buf_virt, u64 buf_phys) {
    nvme_command_t cmd {};
    cmd.cdw0 = NVME_OPC_IDENTIFY;
    cmd.nsid = nsid;
    cmd.prp1 = buf_phys;
    cmd.cdw10 = cns;

    mutex_lock(&ctx->submit_mutex);
    bool ok = nvme_submit_and_wait(ctx, &ctx->admin_q, cmd);
    mutex_unlock(&ctx->submit_mutex);
    return ok;
}

bool nvme_create_io_queues(nvme_driver_ctx_t* ctx, u16 qid, u16 depth) {
    if (!nvme_queue_alloc(ctx, &ctx->io_q, qid, depth))
        return false;

    {
        nvme_command_t cmd {};
        cmd.cdw0 = NVME_OPC_CREATE_CQ;
        cmd.prp1 = ctx->io_q.cq_phys;
        cmd.cdw10 = ((u32)(depth - 1) << 16) | qid;
        cmd.cdw11 = 1;

        mutex_lock(&ctx->submit_mutex);
        bool ok = nvme_submit_and_wait(ctx, &ctx->admin_q, cmd);
        mutex_unlock(&ctx->submit_mutex);
        if (!ok)
            return false;
    }

    {
        nvme_command_t cmd {};
        cmd.cdw0 = NVME_OPC_CREATE_SQ;
        cmd.prp1 = ctx->io_q.sq_phys;
        cmd.cdw10 = ((u32)(depth - 1) << 16) | qid;
        cmd.cdw11 = ((u32)qid << 16) | 1;

        mutex_lock(&ctx->submit_mutex);
        bool ok = nvme_submit_and_wait(ctx, &ctx->admin_q, cmd);
        mutex_unlock(&ctx->submit_mutex);
        if (!ok)
            return false;
    }

    return true;
}

bool nvme_init(const pci_device_t* device, nvme_driver_ctx_t* ctx, std::dynamic_array<nvme_device_t>* device_list) {
    if (!device)
        return false;

    pci_cmd_enable(device, PCI_CMD_MMIO | PCI_CMD_BUS_MASTERING);

    ctx->dma = dma_heap_manager_create_heap(get_global_dma_heap_manager(), PAGE_SIZE_LARGE);

    const u64 mmio_addr_physical_low = pci_read_bar(device, 0) & ~0xFull;
    const u64 mmio_addr_physical_high = pci_read_bar(device, 1);
    const u64 nvme_mmio = mmio_addr_physical_low | (mmio_addr_physical_high << 32);

    ctx->regs = (volatile nvme_regs_t*)vmem_map_mmio_region((void*)nvme_mmio);
    if (!ctx->regs)
        return false;

    mutex_init(&ctx->submit_mutex);

    const u64 cap = ctx->regs->cap;
    ctx->doorbell_stride = 4u << NVME_CAP_DSTRD(cap);

    ctx->regs->cc &= ~NVME_CC_EN;
    if (!nvme_poll_until(&ctx->regs->csts, NVME_CSTS_RDY, 0))
        return false;

    const u16 admin_depth = MAX(2, (u16)((NVME_CAP_MQES(cap) + 1 < 64) ? (NVME_CAP_MQES(cap) + 1) : 64));
    if (!nvme_queue_alloc(ctx, &ctx->admin_q, 0, admin_depth))
        return false;

    ctx->regs->aqa = ((u32)(admin_depth - 1) << 16) | (u32)(admin_depth - 1);
    ctx->regs->asq = ctx->admin_q.sq_phys;
    ctx->regs->acq = ctx->admin_q.cq_phys;

    ctx->regs->cc = NVME_CC_EN | NVME_CC_CSS_NVM | NVME_CC_MPS(1) | NVME_CC_IOSQES(6) | NVME_CC_IOCQES(4);
    for (int i = 0; i < 1000; i++) amd64_pause();
    if (!nvme_poll_until(&ctx->regs->csts, NVME_CSTS_RDY, NVME_CSTS_RDY))
        return false;

    if (ctx->regs->csts & NVME_CSTS_CFS)
        return false;

    u64 identify_phys = 0;
    void* identify_buf = nvme_dma_alloc(ctx->dma, 4096, &identify_phys, 4096);
    if (!identify_buf)
        return false;
    if (!nvme_identify(ctx, 0, NVME_CNS_IDENTIFY_CTRL, identify_buf, identify_phys))
        return false;

    u64 nslist_phys = 0;
    u32* nslist = (u32*)nvme_dma_alloc(ctx->dma, 4096, &nslist_phys, 4096);
    if (!nslist)
        return false;

    if (!nvme_identify(ctx, 0, NVME_CNS_ACTIVE_NS_LIST, nslist, nslist_phys))
        return false;

    const u16 io_depth = admin_depth;
    if (!nvme_create_io_queues(ctx, 1, io_depth))
        return false;

    u64 ns_identify_phys = 0;
    u8* ns_identify_buf = (u8*)nvme_dma_alloc(ctx->dma, 4096, &ns_identify_phys, 4096);
    if (!ns_identify_buf)
        return false;

    // TODO @since 27/08/2026 -- 03:54
    // nslist == empty???

    for (u32 i = 0; i < 1024 && nslist[i] != 0; i++) {
        const u32 nsid = nslist[i];

        if (!nvme_identify(ctx, nsid, NVME_CNS_IDENTIFY_NS, ns_identify_buf, ns_identify_phys))
            continue;

        const u64 nsze = *(volatile u64*)(ns_identify_buf + 0);
        const u8 flbas = *(volatile u8*)(ns_identify_buf + 26) & 0xF;
        const u32 lbaf = *(volatile u32*)(ns_identify_buf + 128 + flbas * 4);
        const u8 lbads = (u8)((lbaf >> 16) & 0xFF);

        nvme_device_t nvme_device {};
        nvme_device.nvme_driver_ctx = ctx;
        nvme_device.nsid = nsid;
        nvme_device.block_count = nsze;
        nvme_device.block_size = 1u << lbads;

        device_list->insert_back(nvme_device);
    }

    return true;
}

bool is_nvme_device(const pci_device_t* device) {
    return device->class_info.prog_if == 0x02 &&
           device->class_info.class_code == 0x01 &&
           device->class_info.sub_class == 0x08;
}