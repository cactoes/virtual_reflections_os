#include "drivers/network/e1000.hpp"
#include "memory/heap.hpp"

#include "memory/vmem.hpp"
#include "interrupt_manager.hpp"
#include "io.hpp"

#include "arch/arch_selector.hpp"

extern "C" interrupt_t amd64_convert_to_interrupt(u64 code);

#if CPU_ARCHITECTURE == ARCH_AMD64

void* amd64_e1000_handle_interrupt(void* stack, void* data) {
    e1000_generic_handle_interrupt((e1000_t*)data);
    return stack;
}

#else
#error CPU_ARCH_NOT_SUPPORTED
#endif

void e1000_write_reg(e1000_t* p_device, u32 offset, u32 value) {
    *((volatile u32*)((u8*)p_device->mmio_region + offset)) = value;
}

u32 e1000_read_reg(e1000_t* p_device, u32 offset) {
    return *((volatile u32*)((u8*)p_device->mmio_region + offset));
}

bool e1000_load_mac(e1000_t* p_device) {
    u32 ral = e1000_read_reg(p_device, E1000_RAL0);
    u32 rah = e1000_read_reg(p_device, E1000_RAH0);

    if (ral == 0x00000000 || ral == MAX_UINT32 || 
        rah == 0x00000000 || rah == MAX_UINT32 || 
        !(rah & E1000_RAH_AV)) {
        return false;
    }
    
    p_device->mac[0] = (ral >> 0) & 0xFF;
    p_device->mac[1] = (ral >> 8) & 0xFF;
    p_device->mac[2] = (ral >> 16) & 0xFF;
    p_device->mac[3] = (ral >> 24) & 0xFF;
    p_device->mac[4] = (rah >> 0) & 0xFF;
    p_device->mac[5] = (rah >> 8) & 0xFF;

    return true;
}

int e1000_receive_init(e1000_t* p_device) {
    p_device->rdesc_array = (e1000_rdesc_t*)dma_heap_alloc(p_device->dma_heap, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_rdesc_t), 16);
    if (!p_device->rdesc_array)
        return 1;

    p_device->rdesc_buffer_array = (u8*)dma_heap_alloc(p_device->dma_heap, E1000_RECEIVE_DESC_COUNT * E1000_BUFFER_SIZE, 16);
    if (!p_device->rdesc_buffer_array)
        return 2;

    for (int i = 0; i < E1000_RECEIVE_DESC_COUNT; i++) {
        memzero(&p_device->rdesc_array[i], sizeof(e1000_rdesc_t));
        p_device->rdesc_array[i].buffer_addr = dma_get_physical(p_device->dma_heap, (p_device->rdesc_buffer_array + i * E1000_BUFFER_SIZE));
    }

    u64 physical = (u64)dma_get_physical(p_device->dma_heap, p_device->rdesc_array);
    if (physical == 0)
        return 3;

    e1000_write_reg(p_device, E1000_RDBAL, (u32)physical);
    e1000_write_reg(p_device, E1000_RDBAH, (u32)(physical >> 32));
    e1000_write_reg(p_device, E1000_RDLEN, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_rdesc_t));
    e1000_write_reg(p_device, E1000_RDH, 0);
    e1000_write_reg(p_device, E1000_RDT, E1000_RECEIVE_DESC_COUNT - 1);

    u32 rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_SZ_2048;
    e1000_write_reg(p_device, E1000_RCTL, rctl);

    p_device->rx_tail = 0;

    return 0;
}

int e1000_transmit_init(e1000_t* p_device) {
    p_device->tdesc_array = (e1000_tdesc_t*)dma_heap_alloc(p_device->dma_heap, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_tdesc_t), 16);
    if (!p_device->tdesc_array)
        return 1;

    p_device->tdesc_buffer_array = (u8*)dma_heap_alloc(p_device->dma_heap, E1000_TRANSMIT_DESC_COUNT * E1000_BUFFER_SIZE, 16);
    if (!p_device->tdesc_buffer_array)
        return 2;
    
    for (int i = 0; i < E1000_TRANSMIT_DESC_COUNT; i++) {
        memzero(&p_device->tdesc_array[i], sizeof(e1000_tdesc_t));
        p_device->tdesc_array[i].buffer_addr = dma_get_physical(p_device->dma_heap, (p_device->tdesc_buffer_array + i * E1000_BUFFER_SIZE));
    }

    u64 physical = (u64)dma_get_physical(p_device->dma_heap, p_device->tdesc_array);
    if (physical == 0)
        return 3;

    e1000_write_reg(p_device, E1000_TDBAL, (u32)physical);
    e1000_write_reg(p_device, E1000_TDBAH, (u32)(physical >> 32));
    e1000_write_reg(p_device, E1000_TDLEN, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_tdesc_t));
    e1000_write_reg(p_device, E1000_TDH, 0);
    e1000_write_reg(p_device, E1000_TDT, 0);

    u32 tctl = E1000_TCTL_EN | E1000_TCTL_PSP | (0x0F << E1000_TCTL_CT_SHIFT) | (0x40 << E1000_TCTL_COLD_SHIFT);
    e1000_write_reg(p_device, E1000_TCTL, tctl);

    p_device->tx_tail = 0;
    
    return 0;
}

// BUG @since 13/05/2026 -- 20:14
// DISABLE_SSE is a temp fix for some sse2 bug
DISABLE_SSE void e1000_recieve_packet(e1000_t* device) {
    // get current desc
    e1000_rdesc_t* desc = &device->rdesc_array[device->rx_tail];

    // check if packet is ready
    while (desc->status & E1000_RDESC_STATUS_DONE) {
        u8* packet = device->rdesc_buffer_array + (device->rx_tail * E1000_BUFFER_SIZE);
        size_t length = desc->length;

        network_packet_t network_packet {};
        network_packet.interface = nic_get_interface_from_device(get_global_nic(), device);
        network_packet.data = (u8*)malloc(length);
        memcpy(network_packet.data, packet, length);
        network_packet.size = length;
        nic_receive_packet(get_global_nic(), network_packet);

        desc->status = 0;
        desc->length = 0;
        desc->checksum = 0;
        desc->errors = 0;

        device->rx_tail = (device->rx_tail + 1) % E1000_RECEIVE_DESC_COUNT;
        desc = &device->rdesc_array[device->rx_tail];
    }
    
    u16 rdt_value = (device->rx_tail + E1000_RECEIVE_DESC_COUNT - 1) % E1000_RECEIVE_DESC_COUNT;
    e1000_write_reg(device, E1000_RDT, rdt_value);
}

void e1000_generic_handle_interrupt(e1000_t* device) {
    if (!device)
        return;

    // get & clear the interrupt
    u32 icr = e1000_read_reg(device, E1000_ICR);

    // packet recieved interrupt
    if (icr & (E1000_IMS_RXT0 | E1000_IMS_RXDMT0))
        e1000_recieve_packet(device);

    // link status changed interrupt
    if (icr & E1000_IMS_LSC) {
        u32 status = e1000_read_reg(device, E1000_STATUS);
        kprintf("Link status changed: 0x%uh\n", status);
    }
}

void e1000_enable_interrupts(e1000_t* device) {
    // clear interrupts
    e1000_read_reg(device, E1000_ICR);

    // enable interrupts
    u32 interrupt_mask = E1000_IMS_RXT0 | E1000_IMS_RXDMT0 | E1000_IMS_RXSEQ | E1000_IMS_LSC;
    e1000_write_reg(device, E1000_IMS, interrupt_mask);
}

bool e1000_init_device(const pci_device_t* pcie_device, e1000_t* network_device) {
    pci_cmd_enable(pcie_device, PCI_CMD_MMIO | PCI_CMD_BUS_MASTERING);

    if (pci_read_bar(pcie_device, 0) & PCI_BAR_IO_REGION)
        return false;

    network_device->dma_heap = dma_heap_manager_create_heap(get_global_dma_heap_manager(), PAGE_SIZE_LARGE);
    if (!network_device->dma_heap)
        return false;

    u64 bar_addr_physical = pci_read_bar(pcie_device, 0) & ~0xF;
    network_device->mmio_region = vmem_map_mmio_region((void*)bar_addr_physical);

    // TODO @since 28/10/2025 -- 01:02
    // free the dma heap
    if (!network_device->mmio_region)
        return false;

    network_device->rx_tail = 0;
    network_device->tx_tail = 0;

    // check status
    const auto status = e1000_read_reg(network_device, E1000_STATUS);
    if ((status == MAX_UINT32) || (status == 0))
        return false;

    if (!(status & E1000_STATUS_EEPROM_PRESENT))
        return false;

    // reset device
    e1000_write_reg(network_device, E1000_CTRL, E1000_CTRL_RST);
    while (e1000_read_reg(network_device, E1000_CTRL) & E1000_CTRL_RST);

    if (!e1000_load_mac(network_device))
        return false;

    // clear & disable interrupts
    e1000_write_reg(network_device, E1000_IMC, MAX_UINT32);
    e1000_read_reg(network_device, E1000_ICR);

    // init receiving packets
    if (e1000_receive_init(network_device) != 0)
        return false;

    // init transmiting packts
    if (e1000_transmit_init(network_device) != 0)
        return false;

    // re-enable specific interrupts
    e1000_enable_interrupts(network_device);

#if CPU_ARCHITECTURE == ARCH_AMD64
    // TODO @since 04/06/2026 -- 00:40
    // MSI

    const u32 irq = pci_config_read(pcie_device, PCI_CONFIG_IRQ_LINE) & MAX_UINT8;
    if (!hook_interrupt(amd64_convert_to_interrupt(irq + 0x20), amd64_e1000_handle_interrupt, (void*)network_device))
        return false;
#else
#error CPU_ARCH_NOT_SUPPORTED
#endif

    // done
    return true;
}

bool e1000_send_packet(e1000_t* device, const void* data, size_t size) {
    if (size > E1000_BUFFER_SIZE)
        return false;

    const u32 next_tail = (device->tx_tail + 1) % E1000_TRANSMIT_DESC_COUNT;
    const u32 head = e1000_read_reg(device, E1000_TDH);
    
    if (next_tail == head)
        return false;

    u8* buffer = device->tdesc_buffer_array + (device->tx_tail * E1000_BUFFER_SIZE);
    memcpy(buffer, data, size);
        
    e1000_tdesc_t* desc = &device->tdesc_array[device->tx_tail];
    desc->length = size;
    desc->cso = 0;
    desc->cmd = E1000_CMD_EOP | E1000_CMD_IFCS | E1000_CMD_RS;
    desc->status = 0;
    desc->css = 0;
    desc->special = 0;

    device->tx_tail = next_tail;
    e1000_write_reg(device, E1000_TDT, device->tx_tail);

    return true;
}

bool is_e1000_device(const pci_device_t* device) {
    return device->vendor_device_id.vendor_id == 0x8086 &&
           device->vendor_device_id.device_id == 0x100E &&
           device->class_info.class_code == 0x2 &&
           device->class_info.sub_class == 0x0;
}