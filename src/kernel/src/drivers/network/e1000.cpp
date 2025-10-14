#include "drivers/network/e1000.hpp"
#include "memory/heap.hpp"
#include "arch/generic.hpp"
#include "memory/vmem.hpp"
#include "interrupt_manager.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

static heap_t g_e1000_dma_heap {};
static e1000_t* g_e1000 = nullptr;

void e1000_write_reg(e1000_t* p_device, uint32_t offset, uint32_t value) {
    *((volatile uint32_t*)((uint8_t*)p_device->mmio_region + offset)) = value;
}

uint32_t e1000_read_reg(e1000_t* p_device, uint32_t offset) {
    return *((volatile uint32_t*)((uint8_t*)p_device->mmio_region + offset));
}

bool e1000_load_mac(e1000_t* p_device) {
    uint32_t ral = e1000_read_reg(p_device, E1000_RAL0);
    uint32_t rah = e1000_read_reg(p_device, E1000_RAH0);

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
    p_device->rdesc_array = (e1000_rdesc_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_rdesc_t), 16);
    if (!p_device->rdesc_array)
        return 1;

    p_device->rdesc_buffer_array = (uint8_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_RECEIVE_DESC_COUNT * E1000_BUFFER_SIZE, 16);
    if (!p_device->rdesc_buffer_array)
        return 2;
    
    for (int i = 0; i < E1000_RECEIVE_DESC_COUNT; ++i) {
        memzero(&p_device->rdesc_array[i], sizeof(e1000_rdesc_t));
        p_device->rdesc_array[i].buffer_addr = dma_get_physical(&g_e1000_dma_heap, (p_device->rdesc_buffer_array + i * E1000_BUFFER_SIZE));
    }

    uint64_t physical = (uint64_t)dma_get_physical(&g_e1000_dma_heap, p_device->rdesc_array);
    if (physical == 0)
        return 3;

    e1000_write_reg(p_device, E1000_RDBAL, (uint32_t)physical);
    e1000_write_reg(p_device, E1000_RDBAH, (uint32_t)(physical >> 32));
    e1000_write_reg(p_device, E1000_RDLEN, E1000_RECEIVE_DESC_COUNT * sizeof(e1000_rdesc_t));
    e1000_write_reg(p_device, E1000_RDH, 0);
    e1000_write_reg(p_device, E1000_RDT, E1000_RECEIVE_DESC_COUNT - 1);

    uint32_t rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_SZ_2048;
    e1000_write_reg(p_device, E1000_RCTL, rctl);

    p_device->rx_tail = 0;

    return 0;
}

int e1000_transmit_init(e1000_t* p_device) {
    p_device->tdesc_array = (e1000_tdesc_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_tdesc_t), 16);
    if (!p_device->tdesc_array)
        return 1;

    p_device->tdesc_buffer_array = (uint8_t*)dma_heap_alloc(&g_e1000_dma_heap, E1000_TRANSMIT_DESC_COUNT * E1000_BUFFER_SIZE, 16);
    if (!p_device->tdesc_buffer_array)
        return 2;
    
    for (int i = 0; i < E1000_TRANSMIT_DESC_COUNT; ++i) {
        memzero(&p_device->tdesc_array[i], sizeof(e1000_tdesc_t));
        p_device->tdesc_array[i].buffer_addr = dma_get_physical(&g_e1000_dma_heap, (p_device->tdesc_buffer_array + i * E1000_BUFFER_SIZE));
    }

    uint64_t physical = (uint64_t)dma_get_physical(&g_e1000_dma_heap, p_device->tdesc_array);
    if (physical == 0)
        return 3;

    e1000_write_reg(p_device, E1000_TDBAL, (uint32_t)physical);
    e1000_write_reg(p_device, E1000_TDBAH, (uint32_t)(physical >> 32));
    e1000_write_reg(p_device, E1000_TDLEN, E1000_TRANSMIT_DESC_COUNT * sizeof(e1000_tdesc_t));
    e1000_write_reg(p_device, E1000_TDH, 0);
    e1000_write_reg(p_device, E1000_TDT, 0);

    uint32_t tctl = E1000_TCTL_EN | E1000_TCTL_PSP | (0x0F << E1000_TCTL_CT_SHIFT) | (0x40 << E1000_TCTL_COLD_SHIFT);
    e1000_write_reg(p_device, E1000_TCTL, tctl);

    p_device->tx_tail = 0;
    
    return 0;
}

void e1000_recieve_packet(e1000_t* p_device) {
    // get current desc
    e1000_rdesc_t* desc = &p_device->rdesc_array[p_device->rx_tail];
    
    // check if packet is ready
    while (desc->status & E1000_RDESC_STATUS_DONE) {
        uint8_t* packet = p_device->rdesc_buffer_array + (p_device->rx_tail * E1000_BUFFER_SIZE);
        size_t length = desc->length;
        
        nidm_packet_recieve_on_device(get_global_nidm(), (void*)p_device, packet, length);
        
        desc->status = 0;
        desc->length = 0;
        desc->checksum = 0;
        desc->errors = 0;
        
        p_device->rx_tail = (p_device->rx_tail + 1) % E1000_RECEIVE_DESC_COUNT;
        desc = &p_device->rdesc_array[p_device->rx_tail];
    }
    
    uint16_t rdt_value = (p_device->rx_tail + E1000_RECEIVE_DESC_COUNT - 1) % E1000_RECEIVE_DESC_COUNT;
    e1000_write_reg(p_device, E1000_RDT, rdt_value);
}

void e1000_enable_interrupts(e1000_t* p_device) {
    // clear interrupts
    e1000_read_reg(p_device, E1000_ICR);

    // enable interrupts
    uint32_t interrupt_mask = E1000_IMS_RXT0 | E1000_IMS_RXDMT0 | E1000_IMS_RXSEQ | E1000_IMS_LSC;
    e1000_write_reg(p_device, E1000_IMS, interrupt_mask);;
}

int e1000_init_device(const pci_device_t* p_pcie_device, e1000_t* p_network_device) {
    // i dont like this get_pml4,
    // the moment we start switching to virtual envoriments this will likeley break ...
    void* pml4 = get_pml4();

    // register as global e1000 driver
    e1000_set_global_device(p_network_device);

    // enable dma
    auto cmd = pci_config_read(p_pcie_device, PCI_COMMAND);
    cmd |= PCI_CMD_MMIO | PCI_CMD_BUS_MASTERING;
    pci_config_write(p_pcie_device, PCI_COMMAND, cmd);

    if (pci_read_bar(p_pcie_device, 0) & PCI_BAR_MMIO_ENABLED)
        return 1;

    if (dma_heap_init(pml4, &g_e1000_dma_heap, (void*)VMEM_E1000_DMA, PAGE_SIZE_LARGE) != 0)
        return 2;

    // map mmio
    uint64_t bar_addr_physical = pci_read_bar(p_pcie_device, 0) & ~0xF;
    uint64_t bar_page_addr_physical = align_down(bar_addr_physical, PAGE_SIZE_LARGE);
    uint64_t bar_addr_offset = bar_addr_physical - bar_page_addr_physical;

    if (!vmem_map_2mb_page(pml4, (void*)VMEM_E1000_MMIO, (void*)bar_page_addr_physical))
        return 3;

    // set vars
    p_network_device->mmio_region = (void*)((uint64_t)VMEM_E1000_MMIO + bar_addr_offset);
    p_network_device->rx_tail = 0;
    p_network_device->tx_tail = 0;

    // check status
    const auto status = e1000_read_reg(p_network_device, E1000_STATUS);
    if ((status == MAX_UINT32) || (status == 0))
        return 4;

    if (!(status & E1000_STATUS_EEPROM_PRESENT))
        return 5;

    // reset device
    e1000_write_reg(p_network_device, E1000_CTRL, E1000_CTRL_RST);
    while (e1000_read_reg(p_network_device, E1000_CTRL) & E1000_CTRL_RST);

    if (!e1000_load_mac(p_network_device))
        return 6;

    // clear & disable interrupts
    e1000_write_reg(p_network_device, E1000_IMC, MAX_UINT32);
    e1000_read_reg(p_network_device, E1000_ICR);

    // init receiving packets
    if (e1000_receive_init(p_network_device) != 0)
        return 7;

    // init transmiting packts
    if (e1000_transmit_init(p_network_device) != 0)
        return 8;

    // re-enable specific interrupts
    e1000_enable_interrupts(p_network_device);

    const uint32_t irq = pci_config_read(p_pcie_device, PCI_CONFIG_IRQ_LINE) & MAX_UINT8;
    if (!set_interrupt_callback(convert_to_interrupt(interrupt_irq_to_int(irq)), e1000_handle_interrupt))
        return 9;

    // done
    return 0;
}

cpu_state_t* e1000_handle_interrupt(cpu_state_t* p_rsp) {
    auto p_device = e1000_get_global_device();

    // get & clear the interrupt
    uint32_t icr = e1000_read_reg(p_device, E1000_ICR);

    // packet recieved interrupt
    if (icr & (E1000_IMS_RXT0 | E1000_IMS_RXDMT0))
        e1000_recieve_packet(p_device);

    // link status changed interrupt
    if (icr & E1000_IMS_LSC) {
        uint32_t status = e1000_read_reg(p_device, E1000_STATUS);
        printf(DBG, "Link status changed: 0x%uh\n", status);
    }

    return p_rsp;
}

int e1000_send_packet(e1000_t* p_device, const void* data, size_t size) {
    if (size > E1000_BUFFER_SIZE)
        return 1;

    const uint32_t next_tail = (p_device->tx_tail + 1) % E1000_TRANSMIT_DESC_COUNT;
    const uint32_t head = e1000_read_reg(p_device, E1000_TDH);
    
    if (next_tail == head)
        return 2;

    uint8_t* buffer = p_device->tdesc_buffer_array + (p_device->tx_tail * E1000_BUFFER_SIZE);
    memcpy(buffer, data, size);
        
    e1000_tdesc_t* desc = &p_device->tdesc_array[p_device->tx_tail];
    desc->length = size;
    desc->cso = 0;
    desc->cmd = E1000_CMD_EOP | E1000_CMD_IFCS | E1000_CMD_RS;
    desc->status = 0;
    desc->css = 0;
    desc->special = 0;

    p_device->tx_tail = next_tail;
    e1000_write_reg(p_device, E1000_TDT, p_device->tx_tail);

    return 0;
}

e1000_t* e1000_get_global_device() {
    return g_e1000;
}

void e1000_set_global_device(e1000_t* p_device) {
    g_e1000 = p_device;
}

int e1000_nidm_send_packet(network_interface_device_t* p_nid, const void* data, size_t size) {
    return e1000_send_packet((e1000_t*)p_nid->device_data, data, size);
}