#include "drivers/network/rtl8168.hpp"
#include "memory/vmem.hpp"
#include "arch/amd64/cpu.hpp"
#include "io.hpp"
#include "virtual_thread.hpp"

void rtl8168_write_reg32(rtl8168_t* device, u32 offset, u32 value) {
    *((volatile u32*)((u8*)device->mmio_region + offset)) = value;
}

u32 rtl8168_read_reg32(rtl8168_t* device, u32 offset) {
    return *((volatile u32*)((u8*)device->mmio_region + offset));
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

bool rtl8168_init_device(const pci_device_t* pcie_device, rtl8168_t* network_device) {
    auto cmd = pci_config_read(pcie_device, PCI_COMMAND);
    cmd |= PCI_CMD_MMIO | PCI_CMD_BUS_MASTERING;
    pci_config_write(pcie_device, PCI_COMMAND, cmd);
    pci_config_read(pcie_device, PCI_COMMAND);

    if (pci_read_bar(pcie_device, 1) & PCI_BAR_IO_REGION)
        return false;

    amd64_mem_barier();

    network_device->dma_heap = dma_heap_manager_create_heap(get_global_dma_heap_manager(), PAGE_SIZE_LARGE);
    if (!network_device->dma_heap)
        return false;

    u64 mmio_address_physical = pci_read_bar(pcie_device, 1) & ~0xF;
    network_device->mmio_region = vmem_map_mmio_region((void*)mmio_address_physical);
    if (!network_device->mmio_region)
        return false;

    // enable the device
    rtl8168_write_reg8(network_device, RTL8168_CONFIG_1, RTL8168_LWAKE | RTL8168_LWPTN);

    // read mac address
    if (!rtl8168_load_mac(network_device))
        return false;

    // reset device
    rtl8168_write_reg8(network_device, RTL8168_CMD, RTL8168_CMD_RST);
    while (rtl8168_read_reg8(network_device, RTL8168_CMD) & RTL8168_CMD_RST)
        vthread_sleep(1);

    // const u32 irq = pci_config_read(pcie_device, PCI_CONFIG_IRQ_LINE) & MAX_UINT8;
    return true;
}

bool is_rtl8168_device(const pci_device_t* device) {
    return device->vendor_device_id.vendor_id == 0x10ec &&
           device->vendor_device_id.device_id == 0x8168 &&
           device->class_info.class_code == 0x2 &&
           device->class_info.sub_class == 0x0;
}