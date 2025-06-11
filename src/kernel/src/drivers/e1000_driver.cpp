#include "drivers/e1000_driver.hpp"
#include "debug.hpp"
#include "memory.hpp"
#include "cpu.hpp"

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
    
}

int e1000_init(void* pml4, pci_device_info_t* e1000_pci_device, e1000_device_t* device) {
    if (dma_heap_init(pml4, &g_e1000_dma_heap, (void*)E1000_DMA_HEAP_ADDR) != 0)
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

    return 0;
}