#include "drivers/e1000_driver.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "drivers/pit_driver.hpp"

dma_heap_t g_e1000_dma_heap {};

void e1000_write_reg(e1000_device_t* device, uint32_t offset, uint32_t value) {
    *((volatile uint32_t*)((uint8_t*)device->mmio_region + offset)) = value;
}

uint32_t e1000_read_reg(e1000_device_t* device, uint32_t offset) {
    return *((volatile uint32_t*)((uint8_t*)device->mmio_region + offset));
}

uint32_t e1000_eeprom_read(e1000_device_t* device, uint8_t offset) {
    e1000_write_reg(device, E1000_EEPROM_READ_REG, E1000_EEPROM_READ_START | ((uint32_t)offset << 8));

    int retries = 0;
    uint32_t data = 0;
    do {
        data = e1000_read_reg(device, E1000_EEPROM_READ_REG);
        if (++retries > E1000_EEPROM_READ_TIMEOUT)
            return 0xFFFFFFFF;
    } while (!(data & E1000_EEPROM_READ_DONE_BIT));

    return (data >> 16) & 0xFFFF;
}

void e1000_init_mac(e1000_device_t* device) {
    uint32_t ral = *((uint32_t*)((uint8_t*)device->mmio_region + E1000_REG_RAL));
    uint32_t rah = *((uint32_t*)((uint8_t*)device->mmio_region + E1000_REG_RAH));

    if (ral != 0xFFFFFFFF && rah != 0xFFFFFFFF && (rah & (1 << 31))) {
        device->mac[0] = (ral >> 0) & 0xFF;
        device->mac[1] = (ral >> 8) & 0xFF;
        device->mac[2] = (ral >> 16) & 0xFF;
        device->mac[3] = (ral >> 24) & 0xFF;
        device->mac[4] = (rah >> 0) & 0xFF;
        device->mac[5] = (rah >> 8) & 0xFF;

        return;
    }

    // eeprom fallback
    for (size_t i = 0; i < 3; i++) {
        uint32_t mac_part = e1000_eeprom_read(device, i);
        device->mac[i * 2] = mac_part & 0xff;
        device->mac[i * 2 + 1] = (mac_part >> 8) & 0xff;
    }
}

int e1000_transmit_init(e1000_device_t* device) {
    device->transmit_descriptions = (e1000_transmit_desc_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_transmit_desc_t), 128);
    if (!device->transmit_descriptions)
        return 1;

    device->transmit_desc_buffers = (uint8_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_BUFFER_SIZE, 128);
    if (!device->transmit_desc_buffers)
        return 1;
        
    for (int i = 0; i < E1000_TRANSMIT_DESC_COUNT; ++i) {
        device->transmit_descriptions[i].buffer_addr = dma_get_physical(&g_e1000_dma_heap, (device->transmit_desc_buffers + i * E1000_BUFFER_SIZE));
        device->transmit_descriptions[i].cmd = 0;
        device->transmit_descriptions[i].status = (1 << 0);
    }

    void* virt = device->transmit_descriptions;
    uint64_t phys = dma_get_physical(&g_e1000_dma_heap, virt);
    debug_print("Transmit desc virt: %p, phys: 0x%uh\n", virt, phys);

    uint64_t physical = dma_get_physical(&g_e1000_dma_heap, device->transmit_descriptions);
    debug_print("TDBAL = 0x%uh\n", (uint32_t)(physical));
    debug_print("TDBAH = 0x%uh\n", (uint32_t)(physical >> 32));

    e1000_write_reg(device, E1000_TDBAL, (uint32_t)physical);
    e1000_write_reg(device, E1000_TDBAH, (uint32_t)(physical >> 32));
    e1000_write_reg(device, E1000_TDLEN, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_transmit_desc_t));
    e1000_write_reg(device, E1000_TDH, 0);
    e1000_write_reg(device, E1000_TDT, 0);

    e1000_write_reg(device, E1000_TCTL,
        E1000_TCTL_EN | E1000_TCTL_PSP |
        (0x10 << E1000_TCTL_CT_SHIFT) |
        (0x40 << E1000_TCTL_COLD_SHIFT));

    uint32_t tctl = e1000_read_reg(device, E1000_TCTL);
    debug_print("TCTL: 0x%uh\n", tctl);

    e1000_write_reg(device, E1000_TIPG,
        (10 << 20) | (8 << 10) | 6);

    return 0;
}

int e1000_receive_init(e1000_device_t* device) {
    device->receive_descriptions = (e1000_receive_desc_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_receive_desc_t), 16);
    if (!device->receive_descriptions)
        return 1;

    device->receive_desc_buffers = (uint8_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_RECEIVE_DESC_COUNT * E1000_BUFFER_SIZE, 16);
    if (!device->receive_desc_buffers)
        return 1;

    for (int i = 0; i < E1000_RECEIVE_DESC_COUNT; ++i) {
        device->receive_descriptions[i].buffer_addr = dma_get_physical(&g_e1000_dma_heap, (device->receive_desc_buffers + i * E1000_BUFFER_SIZE));
        device->receive_descriptions[i].status = 0;
    }

    uint64_t physical = (uint64_t)dma_get_physical(&g_e1000_dma_heap, device->receive_descriptions);
    e1000_write_reg(device, E1000_RDBAL, (uint32_t)physical);
    e1000_write_reg(device, E1000_RDBAH, (uint32_t)(physical >> 32));
    e1000_write_reg(device, E1000_RDLEN, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_receive_desc_t));
    e1000_write_reg(device, E1000_RDH, 0);
    e1000_write_reg(device, E1000_RDT, E1000_RECEIVE_DESC_COUNT - 1);

    uint32_t rctl = e1000_read_reg(device, 0x0100);
    rctl |= E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_SZ_2048;
    rctl &= ~(E1000_RCTL_UPE | E1000_RCTL_MPE);
    e1000_write_reg(device, 0x0100, rctl);

    return 0;
}

int e1000_send_packet(e1000_device_t* device, const void* data, size_t size) {
    if (size > E1000_BUFFER_SIZE)
        return 1;

    uint32_t tdt = e1000_read_reg(device, E1000_TDT);
    e1000_transmit_desc_t* desc = &device->transmit_descriptions[tdt];

    if (!(desc->status & (1 << 0)))
        return 2;

    memcpy(device->transmit_desc_buffers + (tdt * E1000_BUFFER_SIZE), data, size);

    desc->length = size;
    desc->cmd = (1 << 0) | (1 << 1) | (1 << 3);
    desc->status = 0;

    __asm__ volatile("" ::: "memory");

    e1000_write_reg(device, E1000_TDT, (tdt + 1) % E1000_TRANSMIT_DESC_COUNT);

    for (int i = 0; i < 1000000; ++i) {
        if (desc->status & (1 << 0)) {
            debug_print("TX complete!\n");
            break;
        }
    }

    debug_print("loop done!\n");

    return 0;
}

int e1000_init(void* pml4, pci_device_info_t* e1000_pci_device, e1000_device_t* device) {
    if (dma_heap_init(pml4, &g_e1000_dma_heap, (void*)E1000_DMA_HEAP_ADDR, PAGE_SIZE_LARGE) != 0)
        return 1;

    uint64_t bar_addr_physical = e1000_pci_device->bar0_address & ~0xF;
    uint64_t bar_page_addr_physical = mem_align_down(bar_addr_physical, PAGE_SIZE_LARGE);
    uint64_t bar_addr_offset = bar_addr_physical - bar_page_addr_physical;

    if (!vmem_map_2mb_page(pml4, (void*)E1000_MMIO_ADDR, (void*)bar_page_addr_physical))
        return 2;

    device->mmio_region = (uint32_t*)((uint32_t)E1000_MMIO_ADDR + bar_addr_offset);

    uint32_t status = e1000_read_reg(device, E1000_STATUS);
    if ((status == 0xFFFFFFFF) || (status == 0))
        return 3;

    if (!(status & E1000_STATUS_EEPROM_PRESENT))
        return 4;

    e1000_init_mac(device);
    e1000_transmit_init(device);
    e1000_receive_init(device);

    // uint8_t test_packet[60] = {
    //     // Destination MAC (broadcast)
    //     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    //     // Source MAC (your MAC)
    //     e1000_device.mac[0], e1000_device.mac[1], e1000_device.mac[2], e1000_device.mac[3], e1000_device.mac[4], e1000_device.mac[5], e1000_device.mac[6],
    //     // Ethertype (0x0800 = IPv4)
    //     0x08, 0x00,
    //     // Payload (fill with test data, must be ≥ 46 bytes total)
    //     'T','E','S','T','_','P','A','C','K','E','T','_','D','A','T','A',
    //     '1','2','3','4','5','6','7','8','9','0','a','b','c','d','e','f',
    //     'g','h','i','j','k','l','m','n','o','p'
    // };

    uint8_t pkt[64] = { 0 };
    memset(pkt, 'A', sizeof(pkt));
    // e1000_send_packet(device, pkt, sizeof(pkt));

    const auto r = e1000_send_packet(device, pkt, sizeof(pkt));
    debug_print("r: %u\n", r);

    uint32_t status2 = e1000_read_reg(device, E1000_STATUS);
    uint32_t icr = e1000_read_reg(device, 0x000C);
    uint32_t tctl = e1000_read_reg(device, E1000_TCTL);

    debug_print("STATUS=0x%uh ICR=0x%uh TCTL=0x%uh\n", status2, icr, tctl);

    return 0;
}