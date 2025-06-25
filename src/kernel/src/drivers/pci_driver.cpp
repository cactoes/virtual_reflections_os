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

bool find_device_class_info(const pci_device_info_t& inf, const pci_device_request_t& req) {
    return (req.class_code == (uint8_t)PCI_UNKNOWN || inf.class_code == req.class_code) &&
           (req.sub_class == (uint8_t)PCI_UNKNOWN || inf.sub_class == req.sub_class) &&
           (req.prog_if == (uint8_t)PCI_UNKNOWN || inf.prog_if == req.prog_if) &&
           (req.revision_id == (uint8_t)PCI_UNKNOWN || inf.revision_id == req.revision_id);
}

bool find_device_vendor_device_id(const pci_device_info_t& inf, const pci_device_request_t& req) {
    return (req.vendor_id == (uint16_t)PCI_UNKNOWN || inf.vendor_id == req.vendor_id) &&
           (req.device_id == (uint16_t)PCI_UNKNOWN || inf.device_id == req.device_id);
}

bool pci_find_devices(vector<pci_device_info_t>* list, vector<pci_device_request_t>* request_list) {
    if (!list || !request_list)
        return false;

    int found_device_count = 0;

    size_t device_index = 0;
    for (VECTOR_LOOP(list, device_node)) {
        for (VECTOR_LOOP(request_list, request_node)) {
            if (request_node->value.found)
                continue;
            
            bool found = false;

            switch (request_node->value.mode) {
                case pci_device_request_mode_t::CLASS_INFO:
                    found = find_device_class_info(device_node->value, request_node->value);
                    break;
                case pci_device_request_mode_t::VENDOR_DEVICE_ID:
                    found = find_device_vendor_device_id(device_node->value, request_node->value);
                    break;
            }

            if (found) {
                found_device_count++;
                request_node->value.vendor_id = device_node->value.vendor_id;
                request_node->value.device_id = device_node->value.device_id;
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