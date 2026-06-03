#include "drivers/network/rtl8168.hpp"
#include "memory/vmem.hpp"
#include "arch/amd64/cpu.hpp"
#include "io.hpp"
#include "virtual_thread.hpp"
#include "interrupt_manager.hpp"
#include "network/nidm.hpp"

#include "arch/arch_selector.hpp"

#if CPU_ARCHITECTURE == ARCH_AMD64

extern "C" interrupt_t amd64_convert_to_interrupt(u64 code);

void* amd64_rtl8168_handle_interrupt(void* stack, void* data) {
    rtl8168_generic_handle_interrupt((rtl8168_t*)data);
    return stack;
}

#else
#error CPU_ARCH_NOT_SUPPORTED
#endif

void rtl8168_write_reg32(rtl8168_t* device, u32 offset, u32 value) {
    *((volatile u32*)((u8*)device->mmio_region + offset)) = value;
}

u32 rtl8168_read_reg32(rtl8168_t* device, u32 offset) {
    return *((volatile u32*)((u8*)device->mmio_region + offset));
}

void rtl8168_write_reg16(rtl8168_t* device, u32 offset, u16 value) {
    *((volatile u16*)((u8*)device->mmio_region + offset)) = value;
}

u16 rtl8168_read_reg16(rtl8168_t* device, u32 offset) {
    return *((volatile u16*)((u8*)device->mmio_region + offset));
}

void rtl8168_write_reg8(rtl8168_t* device, u32 offset, u8 value) {
    *((volatile u8*)((u8*)device->mmio_region + offset)) = value;
}

u8 rtl8168_read_reg8(rtl8168_t* device, u32 offset) {
    return *((volatile u8*)((u8*)device->mmio_region + offset));
}

bool rtl8168_load_mac(rtl8168_t* device) {
    if (!device)
        return false;

    u32 mac_low = rtl8168_read_reg32(device, RTL8168_MAC);
    u32 mac_high = rtl8168_read_reg32(device, RTL8168_MAC + sizeof(u32));

    if (mac_low == 0 || mac_low == MAX_UINT32 ||
        mac_high == 0 || mac_high == MAX_UINT32)
        return false;

    device->mac[0] = (mac_low >> 0) & 0xff;
    device->mac[1] = (mac_low >> 8) & 0xff;
    device->mac[2] = (mac_low >> 16) & 0xff;
    device->mac[3] = (mac_low >> 24) & 0xff;
    device->mac[4] = (mac_high >> 0) & 0xff;
    device->mac[5] = (mac_high >> 8) & 0xff;

    return true;
}

bool rtl8168_receive_init(rtl8168_t* network_device) {
    if (!network_device)
        return false;

    network_device->rx_current = 0;

    network_device->rdesc_array = (rtl8168_desc_t*)dma_heap_alloc(network_device->dma_heap, sizeof(rtl8168_desc_t) * NUM_RX_DESC, 16);
    if (!network_device->rdesc_array)
        return false;

    memzero(network_device->rdesc_array, sizeof(rtl8168_desc_t) * NUM_RX_DESC);

    network_device->rx_buffer_array = (void*)dma_heap_alloc(network_device->dma_heap, RX_BUF_SIZE * NUM_RX_DESC, 16);
    if (!network_device->rx_buffer_array)
        return false;

    memzero(network_device->rx_buffer_array, RX_BUF_SIZE * NUM_RX_DESC);

    for (u64 i = 0; i < NUM_RX_DESC; i++) {
        u8* buffer = ((u8*)network_device->rx_buffer_array) + RX_BUF_SIZE * i;

        u64 physical = (u64)dma_get_physical(network_device->dma_heap, buffer);

        network_device->rdesc_array[i].vlan = 0;
        network_device->rdesc_array[i].address_low = (u32)physical;
        network_device->rdesc_array[i].address_high = (u32)(physical >> 32);

        u32 cmd = RX_BUF_SIZE | RTL8168_DESC_OWN;
        if (i == NUM_RX_DESC - 1)
            cmd |= RTL8168_DESC_EOR;

        network_device->rdesc_array[i].command = cmd;
    }

    // upload physical address
    u64 physical = (u64)dma_get_physical(network_device->dma_heap, network_device->rdesc_array);
    rtl8168_write_reg32(network_device, RTL8168_RX_DESC_ADDR_LOW, (u32)physical);
    rtl8168_write_reg32(network_device, RTL8168_RX_DESC_ADDR_HIGH, (u32)(physical >> 32));

    // upload buffer size
    rtl8168_write_reg16(network_device, RTL8168_RX_MAX_SIZE, RX_BUF_SIZE);

    // upload settings
    rtl8168_write_reg32(network_device, RTL8168_RX_CONFIG, 0x0f | 0x0700);

    return true;
}

bool rtl8168_transmit_init(rtl8168_t* network_device) {
    if (!network_device)
        return false;

    network_device->tx_current = 0;

    network_device->tdesc_array = (rtl8168_desc_t*)dma_heap_alloc(network_device->dma_heap, sizeof(rtl8168_desc_t) * NUM_TX_DESC, 16);
    if (!network_device->tdesc_array)
        return false;

    memzero(network_device->tdesc_array, sizeof(rtl8168_desc_t) * NUM_TX_DESC);

    network_device->tx_buffer_array = (void*)dma_heap_alloc(network_device->dma_heap, TX_BUF_SIZE * NUM_TX_DESC, 16);
    if (!network_device->tx_buffer_array)
        return false;

    memzero(network_device->tx_buffer_array, TX_BUF_SIZE * NUM_TX_DESC);

    for (u64 i = 0; i < NUM_TX_DESC; i++) {
        u8* buffer = ((u8*)network_device->tx_buffer_array) + TX_BUF_SIZE * i;

        u64 physical = (u64)dma_get_physical(network_device->dma_heap, buffer);

        network_device->tdesc_array[i].vlan = 0;
        network_device->tdesc_array[i].address_low = (u32)physical;
        network_device->tdesc_array[i].address_high = (u32)(physical >> 32);

        u32 cmd = 0;
        if (i == NUM_TX_DESC - 1)
            cmd |= RTL8168_DESC_EOR;

        network_device->tdesc_array[i].command = cmd;
    }

    // upload physical address
    u64 physical = (u64)dma_get_physical(network_device->dma_heap, network_device->tdesc_array);
    rtl8168_write_reg32(network_device, RTL8168_TX_DESC_ADDR_LOW, (u32)physical);
    rtl8168_write_reg32(network_device, RTL8168_TX_DESC_ADDR_HIGH, (u32)(physical >> 32));

    // upload settings
    rtl8168_write_reg32(network_device, RTL8168_TX_CONFIG, 0x03000700);

    return true;
}

bool rtl8168_init_device(const pci_device_t* pcie_device, rtl8168_t* network_device) {
    auto cmd = pci_config_read(pcie_device, PCI_COMMAND);
    cmd |= PCI_CMD_MMIO | PCI_CMD_BUS_MASTERING;
    pci_config_write(pcie_device, PCI_COMMAND, cmd);

    u32 bar2_low = pci_config_read(pcie_device, PCI_GET_BAR_OFFSET(2));
    u64 physical_mmio_base = bar2_low & 0xFFFFFFF0;
    
    int is_64_bit = ((bar2_low & 0x6) == 0x4);
    u32 bar3_high = 0;

    if (is_64_bit) {
        bar3_high = pci_config_read(pcie_device, PCI_GET_BAR_OFFSET(3));
        physical_mmio_base |= ((u64)bar3_high << 32);
    }

    pci_config_write(pcie_device, PCI_GET_BAR_OFFSET(2), 0xFFFFFFFF);
    u32 size_mask_low = pci_config_read(pcie_device, PCI_GET_BAR_OFFSET(2));
    
    pci_config_write(pcie_device, PCI_GET_BAR_OFFSET(2), bar2_low);

    u64 size_mask = size_mask_low & 0xFFFFFFF0;

    if (is_64_bit) {
        pci_config_write(pcie_device, PCI_GET_BAR_OFFSET(3), 0xFFFFFFFF);
        u32 size_mask_high = pci_config_read(pcie_device, PCI_GET_BAR_OFFSET(3));
        pci_config_write(pcie_device, PCI_GET_BAR_OFFSET(3), bar3_high);
        
        size_mask |= ((u64)size_mask_high << 32);
    } else {
        size_mask |= 0xFFFFFFFF00000000ULL; 
    }

    u64 bar_size = ~size_mask + 1;

    printf("[ rtl8168 ] device mmio is located at physical address: 0x%uh\n", physical_mmio_base);
    printf("[ rtl8168 ] device mmio size: %uh bytes\n", bar_size);

    amd64_mem_barier();

    network_device->mmio_region = vmem_map_mmio_region((void*)physical_mmio_base);
    if (!network_device->mmio_region)
        return false;

    network_device->dma_heap = dma_heap_manager_create_heap(get_global_dma_heap_manager(), PAGE_SIZE_LARGE);
    if (!network_device->dma_heap)
        return false;

    // enable the device
    rtl8168_write_reg8(network_device, RTL8168_CONFIG_1, RTL8168_LWAKE | RTL8168_LWPTN);

    // read mac address
    if (!rtl8168_load_mac(network_device))
        return false;

    printf("[ rtl8168 ] loaded MAC: %uh:%uh:%uh:%uh:%uh:%uh\n", network_device->mac[0], network_device->mac[1], network_device->mac[2], network_device->mac[3], network_device->mac[4], network_device->mac[5]);

    // reset device
    rtl8168_write_reg8(network_device, RTL8168_CMD, RTL8168_CMD_RST);
    while (rtl8168_read_reg8(network_device, RTL8168_CMD) & RTL8168_CMD_RST)
        vthread_sleep(1);

    printf("[ rtl8168 ] device reset succesfull\n" );

    rtl8168_write_reg8(network_device, RTL8168_CFG9346, RTL8168_CFG9346_UNLOCK);

    if (!rtl8168_receive_init(network_device))
        return false;

    printf("[ rtl8168 ] initialized RX\n" );

    if (!rtl8168_transmit_init(network_device))
        return false;

    printf("[ rtl8168 ] initialized TX\n" );

    rtl8168_write_reg16(network_device, RTL8168_IMR, 0x003F);

    rtl8168_write_reg8(network_device, RTL8168_CMD, RTL8168_CMD_TX_EN | RTL8168_CMD_RX_EN);

    rtl8168_write_reg8(network_device, RTL8168_CFG9346, RTL8168_CFG9346_LOCK);

    // TODO @since 03/06/2026 -- 21:58
    // bind MSI

// #if CPU_ARCHITECTURE == ARCH_AMD64
//     const u32 irq = pci_config_read(pcie_device, PCI_CONFIG_IRQ_LINE) & MAX_UINT16;
//     printf("[ rtl8168 ] irq line %u\n", irq);
//     if (!hook_interrupt(amd64_convert_to_interrupt(irq + 0x20), amd64_rtl8168_handle_interrupt, (void*)network_device))
//         return false;
// #else
// #error CPU_ARCH_NOT_SUPPORTED
// #endif

    printf("[ rtl8168 ] finished initialization\n" );

    return true;
}

bool rtl8168_send_packet(rtl8168_t* device, const void* data, u64 size) {
    if (!device || !data || size > TX_BUF_SIZE)
        return false;

    u64 index = device->tx_current;

    // last packet still busy
    if (device->tdesc_array[index].command & RTL8168_DESC_OWN)
        // we could wait?
        return false;

    u8* buffer = ((u8*)device->tx_buffer_array) + (TX_BUF_SIZE * index);
    memcpy(buffer, data, size);

    u32 cmd = (u32)size | RTL8168_DESC_OWN | RTL8168_DESC_FS | RTL8168_DESC_LS; 

    if (index == NUM_TX_DESC - 1)
        cmd |= RTL8168_DESC_EOR;

    device->tdesc_array[index].command = cmd;

    rtl8168_write_reg8(device, RTL8168_TPPOLL, (1 << 6));

    device->tx_current = (index + 1) % NUM_TX_DESC;

    return true;
}

DISABLE_SSE void rtl8168_receive_packet(rtl8168_t* device) {
    while ((device->rdesc_array[device->rx_current].command & RTL8168_DESC_OWN) == 0) {
        u32 cmd = device->rdesc_array[device->rx_current].command;
        u32 length = cmd & 0x3FFF;
        length -= 4;
        u8* packet = ((u8*)device->rx_buffer_array) + (RX_BUF_SIZE * device->rx_current);

        network_packet_t network_packet {};
        network_packet.interface = nic_get_interface_from_device(get_global_nic(), device);
        network_packet.data = (u8*)malloc(length);
        memcpy(network_packet.data, packet, length);
        network_packet.size = length;
        nic_receive_packet(get_global_nic(), network_packet);

        u32 new_cmd = RX_BUF_SIZE | RTL8168_DESC_OWN;
        if (device->rx_current == NUM_RX_DESC - 1)
            new_cmd |= RTL8168_DESC_EOR;
        
        device->rdesc_array[device->rx_current].command = new_cmd;

        device->rx_current = (device->rx_current + 1) % NUM_RX_DESC;
    }
}

void rtl8168_generic_handle_interrupt(rtl8168_t* device) {
    if (!device)
        return;

    u16 status = rtl8168_read_reg16(device, RTL8168_ISR);

    if (status == 0)
        return;

    rtl8168_write_reg16(device, RTL8168_ISR, status);

    if (status & RTL8168_ISR_LINKCHG) {
        // TODO @since 03/06/2026 -- 15:14
    }

    if (status & RTL8168_ISR_ROK)
        rtl8168_receive_packet(device);

    if (status & RTL8168_ISR_TOK) {
        // TODO @since 03/06/2026 -- 15:15
        // packet transmit OK
    }

}

bool is_rtl8168_device(const pci_device_t* device) {
    return device->vendor_device_id.vendor_id == 0x10ec &&
           device->vendor_device_id.device_id == 0x8168 &&
           device->class_info.class_code == 0x2 &&
           device->class_info.sub_class == 0x0;
}