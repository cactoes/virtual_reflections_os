#include "driver/pci.hpp"
#include "critical/kernel.hpp"
#include "critical/memory.hpp"
#include "string.hpp"

kresult_t kernel::driver::pci::config_read(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t* result) {
    const uint32_t address = PCI_CREATE_CONFIG_ADDRESS(bus, device, function, offset);
    (void)cpu::out_port(cpu::PT_L, PCI_CONFIG_ADDRESS, address);
    (void)cpu::in_port(cpu::PT_L, PCI_CONFIG_DATA, result);
    return KRESULT(0);
}

kresult_t kernel::driver::pci::config_write(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    const uint32_t address = PCI_CREATE_CONFIG_ADDRESS(bus, device, function, offset);
    (void)cpu::out_port(cpu::PT_L, PCI_CONFIG_ADDRESS, address);
    (void)cpu::out_port(cpu::PT_L, PCI_CONFIG_DATA, value);
    return KRESULT(0);
}

kresult_t kernel::driver::pci::enumerate_pci_devices(pci_device_info_t* pci_list, size_t max_size, size_t* actual_size) {
    *actual_size = 0;

    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t device = 0; device < 32; device++) {
            for (uint32_t function = 0; function < 8; function++) {
                uint32_t vendor_device_id; (void)config_read(bus, device, function, 0x00, &vendor_device_id);
                if (vendor_device_id != PCI_VENDOR_DEVICE_ID_INVALID) {
                    // early return
                    if (*actual_size >= max_size) {
                        (*actual_size)++;
                        continue;
                    }

                    pci_device_info_t device_info {};
                    device_info.bus = bus;
                    device_info.device = device;
                    device_info.function = function;

                    device_info.vendor_device_id = vendor_device_id;
                    
                    (void)config_read(bus, device, function, 0x08, &device_info.class_info);

                    (void)config_read(bus, device, function, PCI_GET_BAR_OFFSET(0), &device_info.bar0_address);
                    (void)config_read(bus, device, function, PCI_GET_BAR_OFFSET(1), &device_info.bar1_address);
                    (void)config_read(bus, device, function, PCI_GET_BAR_OFFSET(2), &device_info.bar2_address);
                    (void)config_read(bus, device, function, PCI_GET_BAR_OFFSET(3), &device_info.bar3_address);
                    (void)config_read(bus, device, function, PCI_GET_BAR_OFFSET(4), &device_info.bar4_address);
                    (void)config_read(bus, device, function, PCI_GET_BAR_OFFSET(5), &device_info.bar5_address);

                    pci_list[(*actual_size)++] = device_info;
                }
            }
        }
    }

    return *actual_size >= max_size ? KRESULT(1) : KRESULT(0);
}

kresult_t kernel::driver::pci::get_pci_device_count(size_t* device_count) {
    *device_count = 0;

    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t device = 0; device < 32; device++) {
            for (uint32_t function = 0; function < 8; function++) {
                uint32_t vendor_device_id; (void)config_read(bus, device, function, 0x00, &vendor_device_id);
                if (vendor_device_id != PCI_VENDOR_DEVICE_ID_INVALID)
                    (*device_count)++;
            }
        }
    }

    return KRESULT(0);
}

const char* kernel::driver::pci::get_class_description(const pci_device_info_t* pci_device) {
    switch (pci_device->class_code) {
        case 0x01: // Mass Storage Controller
            switch (pci_device->sub_class) {
                case 0x00: return "SCSI Bus Controller";
                case 0x01: return "IDE Controller";
                case 0x02: return "Floppy Disk Controller";
                case 0x03: return "IPI Bus Controller";
                case 0x04: return "RAID Controller";
                case 0x05: return "ATA Controller";
                case 0x06: return "Serial ATA Controller";
                default:   return "Unkown Mass Storage Controller";
            }
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: // Bridge
            switch (pci_device->sub_class) {
                case 0x00: return "Host Bridge";
                case 0x01: return "ISA Bridge";
                case 0x02: return "EISA Bridge";
                case 0x03: return "MCA Bridge";
                case 0x04: return "PCI-to-PCI Bridge";
                case 0x05: return "PCMCIA Bridge";
                case 0x06: return "NuBus Bridge";
                case 0x07: return "CardBus Bridge";
                case 0x08: return "RACEway Bridge";
                case 0x09: return "PCI-to-PCI Bridge";
                default:   return "Unkown Bridge";
            }
        case 0x0C: // Serial Bus Controller
            switch (pci_device->sub_class) {
                case 0x03: return "USB Controller";
                default: return "Unknown Serial Bus Controller";
            }
        case 0x0D: // Wireless Controller
            switch (pci_device->sub_class) {
                case 0x00: return "iRDA Compatible Controller";
                case 0x01: return "Consumer IR Controller";
                case 0x10: return "RF Controller";
                case 0x11: return "Bluetooth Controller";
                case 0x12: return "Broadband Controller";
                case 0x20: return "Ethernet Controller (802.1a)";
                case 0x21: return "Ethernet Controller (802.1b)";
                default:   return "Unkown Wireless Controller";
            }
        default: return "Unknown Device Type";
    }
}

kresult_t kernel::driver::pci::find_pci_devices(const pci_device_info_t* pci_device_list, size_t size, pci_device_request_t* requested_devices, size_t requested_devices_size) {
    if (pci_device_list == nullptr || requested_devices == nullptr)
        return KRESULT(1);

    int found_device_count = 0;

    for (size_t i = 0; i < size; i++) {
        auto& device = pci_device_list[i];

        for (size_t j = 0; j < requested_devices_size; j++) {
            auto& requested_device = requested_devices[j];

            int matching_fields = 0;

            if (device.class_code == requested_device.class_code || requested_device.class_code == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (device.sub_class == requested_device.sub_class || requested_device.sub_class == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (device.prog_if == requested_device.prog_if || requested_device.prog_if == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (device.revision_id == requested_device.revision_id || requested_device.revision_id == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (matching_fields == 4) {
                found_device_count++;
                requested_device.class_info = device.class_info;
                requested_device.pci_device_index = i;
                break;
            }
        }

        if (found_device_count == requested_devices_size)
            return KRESULT(0);
    }

    return KRESULT(2);
}