#include "drivers/network/e1000.hpp"
#include "memory/heap.hpp"
#include "arch/generic.hpp"
#include "memory/vmem.hpp"

static heap_t g_e1000_dma_heap {};

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

int e1000_init_device(const pci_device_t* p_pcie_device, e1000_t* p_network_device) {
    // i dont like this get_pml4,
    // the moment we start switching to virtual envoriments this will likeley break ...
    void* pml4 = get_pml4();

    // enable dma
    auto cmd = pci_config_read(p_pcie_device, 0x04);
    cmd |= 0x06;
    pci_config_write(p_pcie_device, 0x04, cmd);

    if (dma_heap_init(pml4, &g_e1000_dma_heap, (void*)VMEM_E1000_DMA, PAGE_SIZE_LARGE) != 0)
        return 1;

    // map mmio
    uint64_t bar_addr_physical = pci_read_bar(p_pcie_device, 0) & ~0xF;
    uint64_t bar_page_addr_physical = align_down(bar_addr_physical, PAGE_SIZE_LARGE);
    uint64_t bar_addr_offset = bar_addr_physical - bar_page_addr_physical;

    if (!vmem_map_2mb_page(pml4, (void*)VMEM_E1000_MMIO, (void*)bar_page_addr_physical))
        return 2;

    // set vars
    p_network_device->mmio_region = (void*)((uint64_t)VMEM_E1000_MMIO + bar_addr_offset);
    p_network_device->rx_tail = 0;
    p_network_device->tx_tail = 0;

    // check status
    const auto status = e1000_read_reg(p_network_device, E1000_STATUS);
    if ((status == MAX_UINT32) || (status == 0))
        return 3;

    if (!(status & E1000_STATUS_EEPROM_PRESENT))
        return 4;

    // reset device
    e1000_write_reg(p_network_device, E1000_CTRL, E1000_CTRL_RST);
    while (e1000_read_reg(p_network_device, E1000_CTRL) & E1000_CTRL_RST);

    if (!e1000_load_mac(p_network_device))
        return 5;

    return 0;
}