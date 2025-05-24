#include "drivers/pci_driver.hpp"
#include "cpu.hpp"

uint32_t pci_config_read(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    const uint32_t address = PCI_CREATE_CONFIG_ADDRESS(bus, device, function, offset);
    cpu_outl(PCI_CONFIG_ADDRESS, address);
    return cpu_inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint16_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    const uint32_t address = PCI_CREATE_CONFIG_ADDRESS(bus, device, function, offset);
    cpu_outl(PCI_CONFIG_ADDRESS, address);
    cpu_outl(PCI_CONFIG_DATA, value);
}

const char* pci_get_class_description(pci_device_info_t* device) {
    if (!device)
        return "";

    switch (device->class_code) {
        case 0x01: // Mass Storage Controller
            switch (device->sub_class) {
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
            switch (device->sub_class) {
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
            switch (device->sub_class) {
                case 0x03: return "USB Controller";
                default: return "Unknown Serial Bus Controller";
            }
        case 0x0D: // Wireless Controller
            switch (device->sub_class) {
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

bool pci_enumerate_devices(vector<pci_device_info_t>* list) {
    if (!list)
        return false;

    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t device = 0; device < 32; device++) {
            for (uint32_t function = 0; function < 8; function++) {
                uint32_t vendor_device_id = pci_config_read(bus, device, function, 0);
                if (vendor_device_id == PCI_VENDOR_DEVICE_ID_INVALID) {
                    continue;
                }

                pci_device_info_t device_info {};
                device_info.bus = bus;
                device_info.device = device;
                device_info.function = function;
                device_info.vendor_device_id = vendor_device_id;

                device_info.class_info = pci_config_read(bus, device, function, 8);
                
                device_info.bar0_address = pci_config_read(bus, device, function, PCI_GET_BAR_OFFSET(0));
                device_info.bar1_address = pci_config_read(bus, device, function, PCI_GET_BAR_OFFSET(1));
                device_info.bar2_address = pci_config_read(bus, device, function, PCI_GET_BAR_OFFSET(2));
                device_info.bar3_address = pci_config_read(bus, device, function, PCI_GET_BAR_OFFSET(3));
                device_info.bar4_address = pci_config_read(bus, device, function, PCI_GET_BAR_OFFSET(4));
                device_info.bar5_address = pci_config_read(bus, device, function, PCI_GET_BAR_OFFSET(5));

                list->insert_back(device_info);
            }
        }
    }

    return true;
}

bool pci_find_devices(vector<pci_device_info_t>* list, vector<pci_device_request_t>* request_list) {
    if (!list || !request_list)
        return false;

    int found_device_count = 0;

    size_t device_index = 0;
    for (VECTOR_LOOP(list, device_node)) {
        for (VECTOR_LOOP(request_list, request_node)) {
            int matching_fields = 0;

            if (device_node->value.class_code == request_node->value.class_code || request_node->value.class_code == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (device_node->value.sub_class == request_node->value.sub_class || request_node->value.sub_class == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (device_node->value.prog_if == request_node->value.prog_if || request_node->value.prog_if == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (device_node->value.revision_id == request_node->value.revision_id || request_node->value.revision_id == (uint8_t)PCI_UNKNOWN)
                matching_fields++;

            if (matching_fields == 4) {
                found_device_count++;
                request_node->value.class_info = device_node->value.class_info;
                request_node->value.pci_device_index = device_index;
                request_node->value.found = true;
                break;
            }
        }

        if (found_device_count == request_list->length())
            return true;

        device_index++;
    }

    // not all devices were found
    return false;
}
