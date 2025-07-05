#include "drivers/e1000_driver.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "cpu.hpp"
#include "drivers/pit_driver.hpp"
#include "string.hpp"

static dma_heap_t g_e1000_dma_heap {};
static e1000_device_t* g_device;

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

void e1000_write_mac(e1000_device_t* device, uint8_t mac[6]) {
    uint32_t mac_low = 0;
    uint32_t mac_high = 1 << 31;
    memcpy(&mac_low, &mac[0], 4);
    memcpy(&mac_high, &mac[4], 2);

    e1000_write_reg(device, E1000_REG_RAL, mac_low);
    e1000_write_reg(device, E1000_REG_RAH, mac_high);
}


int e1000_transmit_init(e1000_device_t* device) {
    device->transmit_descriptions = (e1000_transmit_desc_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_transmit_desc_t), 16);
    if (!device->transmit_descriptions)
        return 1;

    device->transmit_desc_buffers = (uint8_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_BUFFER_SIZE, 16);
    if (!device->transmit_desc_buffers)
        return 2;
        
    for (int i = 0; i < E1000_TRANSMIT_DESC_COUNT; ++i) {
        memzero(&device->transmit_descriptions[i], sizeof(e1000_transmit_desc_t));
        device->transmit_descriptions[i].buffer_addr = dma_get_physical(&g_e1000_dma_heap, (device->transmit_desc_buffers + i * E1000_BUFFER_SIZE));
        device->transmit_descriptions[i].status = (1 << 0);
    }

    uint64_t physical = dma_get_physical(&g_e1000_dma_heap, device->transmit_descriptions);

    e1000_write_reg(device, E1000_TDBAL, (uint32_t)physical);
    e1000_write_reg(device, E1000_TDBAH, (uint32_t)(physical >> 32));
    e1000_write_reg(device, E1000_TDLEN, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_transmit_desc_t));
    e1000_write_reg(device, E1000_TDH, 0);
    e1000_write_reg(device, E1000_TDT, 0);

    e1000_write_reg(device, E1000_TCTL,
        E1000_TCTL_EN | E1000_TCTL_PSP |
        (0x0F << E1000_TCTL_CT_SHIFT) |
        (0x40 << E1000_TCTL_COLD_SHIFT));

    e1000_write_reg(device, E1000_TIPG, 0x0060200A);

    return 0;
}

int e1000_receive_init(e1000_device_t* device) {
    e1000_write_mac(device, device->mac);

    device->receive_descriptions = (e1000_receive_desc_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_receive_desc_t), 16);
    if (!device->receive_descriptions)
        return 1;

    device->receive_desc_buffers = (uint8_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_RECEIVE_DESC_COUNT * E1000_BUFFER_SIZE, 16);
    if (!device->receive_desc_buffers)
        return 2;

    for (int i = 0; i < E1000_RECEIVE_DESC_COUNT; ++i) {
        memzero(&device->receive_descriptions[i], sizeof(e1000_receive_desc_t));
        device->receive_descriptions[i].buffer_addr = dma_get_physical(&g_e1000_dma_heap, (device->receive_desc_buffers + i * E1000_BUFFER_SIZE));
    }

    uint64_t physical = (uint64_t)dma_get_physical(&g_e1000_dma_heap, device->receive_descriptions);
    e1000_write_reg(device, E1000_RDBAL, (uint32_t)physical);
    e1000_write_reg(device, E1000_RDBAH, (uint32_t)(physical >> 32));
    e1000_write_reg(device, E1000_RDLEN, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_receive_desc_t));
    e1000_write_reg(device, E1000_RDH, 0);
    e1000_write_reg(device, E1000_RDT, E1000_RECEIVE_DESC_COUNT - 1);

    uint32_t rctl = E1000_RCTL_EN |
                    E1000_RCTL_BAM | E1000_RCTL_UPE | E1000_RCTL_MPE |
                    E1000_RCTL_SECRC |
                    E1000_RCTL_SZ_2048 |
                    0x00000000;
    
    e1000_write_reg(device, 0x0100, rctl);

    return 0;
}

int e1000_send_packet(e1000_device_t* device, const void* data, size_t size) {
    if (size > E1000_BUFFER_SIZE)
        return 1;

    uint32_t tail = device->tx_tail;
    e1000_transmit_desc_t* desc = &device->transmit_descriptions[tail];

    if (!(desc->status & (1 << 0)))
        return 2;

    uint8_t* buffer = device->transmit_desc_buffers + (tail * E1000_BUFFER_SIZE);
    memcpy(buffer, data, size);

    desc->length = size;
    desc->cso = 0;
    desc->cmd = E1000_CMD_EOP | E1000_CMD_IFCS | E1000_CMD_RS;
    desc->status = 0;
    desc->css = 0;
    desc->special = 0;

    device->tx_tail = (tail + 1) % E1000_TRANSMIT_DESC_COUNT;
    e1000_write_reg(device, E1000_TDT, device->tx_tail);
    return 0;
}

int e1000_receive_packet(e1000_device_t* device, void* buffer, size_t* packet_size) {
    uint32_t tail = device->rx_tail;
    e1000_receive_desc_t* desc = &device->receive_descriptions[tail];
    
    if (!(desc->status & (1 << 0)))
        return 1;
    
    uint16_t length = desc->length;
    *packet_size = length;
    
    uint8_t* packet_buffer = device->receive_desc_buffers + (tail * E1000_BUFFER_SIZE);
    memcpy(buffer, packet_buffer, length);
    
    desc->status = 0;
    desc->errors = 0;
    desc->length = 0;
    desc->checksum = 0;
    desc->special = 0;
    
    device->rx_tail = (tail + 1) % E1000_RECEIVE_DESC_COUNT;
    e1000_write_reg(device, E1000_RDT, device->rx_tail);
    
    return 0;
}

static inline uint16_t htons16(uint16_t v) { return (v>>8)|(v<<8); }
static inline uint32_t htonl32(uint32_t v) {
    return  (v>>24) | ((v>>8)&0xFF00) | ((v<<8)&0xFF0000) | (v<<24);
}

size_t build_arp_request(const uint8_t mac[6], uint32_t target_ip, uint8_t *buf) {
    /* 0‑13  Ethernet header --------------------------------------------- */
    memset(buf, 0, 60);                          /* zero‑pad to 60 B       */
    memset(buf, 0xFF, 6);                        /* dst = FF:FF:FF:FF:FF:FF*/
    memcpy(buf + 6, mac, 6);                     /* src = our MAC          */
    *(uint16_t*)(buf + 12) = htons16(0x0806);    /* EtherType = ARP        */

    /* 14‑41  ARP payload ------------------------------------------------- */
    uint8_t *arp = buf + 14;
    *(uint16_t*)(arp +  0) = htons16(1);         /* htype = Ethernet       */
    *(uint16_t*)(arp +  2) = htons16(0x0800);    /* ptype = IPv4           */
    arp[4]  = 6;                                 /* hlen  = 6              */
    arp[5]  = 4;                                 /* plen  = 4              */
    *(uint16_t*)(arp +  6) = htons16(1);         /* oper  = REQUEST (1)    */
    memcpy(arp +  8,  mac, 6);                   /* sha   = our MAC        */
    *(uint32_t*)(arp + 14) = htonl32(0);         /* spa   = 0.0.0.0        */
    memset(arp + 18, 0, 6);                      /* tha   = 00:00:..       */
    *(uint32_t*)(arp + 24) = htonl32(target_ip); /* tpa   = target_ip      */

    return 60;                                   /* HW adds the FCS        */
}

int e1000_init(void* pml4, pci_device_info_t* e1000_pci_device, e1000_device_t* device) {
    g_device = device;
    
    uint32_t pci_command = pci_config_read(e1000_pci_device->bus, e1000_pci_device->device, e1000_pci_device->function, 0x04);
    pci_command |= 0x06;
    pci_config_write(e1000_pci_device->bus, e1000_pci_device->device,  e1000_pci_device->function, 0x04, pci_command);
    
    if (dma_heap_init(pml4, &g_e1000_dma_heap, (void*)E1000_DMA_HEAP_ADDR, PAGE_SIZE_LARGE) != 0)
        return 1;

    uint64_t bar_addr_physical = e1000_pci_device->bar0_address & ~0xF;
    uint64_t bar_page_addr_physical = mem_align_down(bar_addr_physical, PAGE_SIZE_LARGE);
    uint64_t bar_addr_offset = bar_addr_physical - bar_page_addr_physical;

    if (!vmem_map_2mb_page(pml4, (void*)E1000_MMIO_ADDR, (void*)bar_page_addr_physical))
        return 2;

    device->mmio_region = (uint32_t*)((uint32_t)E1000_MMIO_ADDR + bar_addr_offset);
    device->tx_tail = 0;
    device->rx_tail = 0;

    uint32_t status = e1000_read_reg(device, E1000_STATUS);
    if ((status == 0xFFFFFFFF) || (status == 0))
        return 3;

    if (!(status & E1000_STATUS_EEPROM_PRESENT))
        return 4;

    e1000_write_reg(device, E1000_CTRL, E1000_CTRL_RST);
    e1000_write_reg(device, 0xD0, (1 << 7) | (1 << 6) | (1 << 4));
    e1000_init_mac(device);

    if (e1000_transmit_init(device) != 0) {
        debug_print("Failed to initialize transmit\n");
        return 5;
    }
    
    if (e1000_receive_init(device) != 0) {
        debug_print("Failed to initialize receive\n");
        return 6;
    }

    uint8_t pkt[60];
    size_t len = build_arp_request(device->mac, 0x0A'00'02'02 /*10.0.2.2*/, pkt);
    int result = e1000_send_packet(device, pkt, len);

    debug_print("send packet result: %u\n", result);

    return 0;
}

cpu_state_t* e1000_handle_interrupt(cpu_state_t* state) {
    int32_t icr = e1000_read_reg(g_device, 0xC0);
    debug_print("e1000 int_handler callback: 0x%uh\n", icr);
    
    e1000_write_reg(g_device, 0xD0, 0x1);

    if ((icr & (1<<6)) || (icr & (1<<7))) {
        uint8_t packet[E1000_BUFFER_SIZE];
        size_t packet_size;

        while (e1000_receive_packet(g_device, packet, &packet_size) == 0) {
            debug_print("Received packet (%u bytes):\n", (uint32_t)packet_size);
            
            // Print Ethernet header
            debug_print("Dst MAC: %uh:%uh:%uh:%uh:%uh:%uh\n", 
                        packet[0], packet[1], packet[2], packet[3], packet[4], packet[5]);
            debug_print("Src MAC: %uh:%uh:%uh:%uh:%uh:%uh\n", 
                        packet[6], packet[7], packet[8], packet[9], packet[10], packet[11]);
            debug_print("EtherType: %uh%uh\n", packet[12], packet[13]);
            
            debug_print("Payload: ");
            size_t print_size = (packet_size > 64) ? 64 : packet_size;
            for (size_t i = 14; i < print_size && i < packet_size; i++) {
                debug_print("%uh ", packet[i]);
                if ((i - 14) % 16 == 15) debug_print("\n         ");
            }
            debug_print("\n");
        }
    }

    e1000_read_reg(g_device, 0xC0);

    return state;
}