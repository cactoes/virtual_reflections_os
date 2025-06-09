#include "drivers/e1000_driver.hpp"
#include "debug.hpp"
#include "memory.hpp"

dma_heap_t g_e1000_dma_heap {};

volatile uint64_t* g_e1000_mmio = nullptr;

void e1000_write_reg(uint32_t offset, uint32_t value) {
    *((volatile uint32_t *)((uint64_t)g_e1000_mmio + offset)) = value;
}

uint32_t e1000_read_reg(uint32_t offset) {
    return *((volatile uint32_t*)((uint64_t)g_e1000_mmio + offset));
}

int e1000_init(void* pml4, pci_device_info_t* e1000_pci_device, e1000_device_t* device) {
    if (dma_heap_init(pml4, &g_e1000_dma_heap, (void*)E1000_DMA_HEAP_ADDR) != 0)
        return 1;

    uint64_t bar_addr_physical = e1000_pci_device->bar0_address & ~0xF;
    uint64_t bar_page_addr_physical = mem_align_down(bar_addr_physical, PAGE_SIZE_LARGE);
    uint64_t bar_addr_offset = bar_addr_physical - bar_page_addr_physical;

    if (!vmem_map_2mb_page(pml4, (void*)E1000_MMIO_ADDR, (void*)bar_page_addr_physical))
        return 2;

    g_e1000_mmio = (volatile uint64_t*)((uint8_t*)E1000_MMIO_ADDR + bar_addr_offset);

    uint32_t status = e1000_read_reg(E1000_STATUS);
    if (!(status & E1000_STATUS_EEPROM_PRESENT)) {
        debug_print("eeprom not present\n");
        return 3;
    }

    debug_print("eeprom present\n");
    return 0;
}