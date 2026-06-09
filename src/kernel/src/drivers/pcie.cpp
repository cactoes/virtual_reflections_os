#include "drivers/pcie.hpp"

// TODO @since 21/05/2026 -- 22:29
// you know the drill
#include "arch/amd64/port.hpp"

pcie_device_manager_t* global_pcie_device_manager = nullptr;

void set_global_pcie_device_manager(pcie_device_manager_t* pcie_device_manager) {
    global_pcie_device_manager = pcie_device_manager;
}

pcie_device_manager_t* get_global_pcie_device_manager() {
    return global_pcie_device_manager;
}

u32 pci_config_read(const pci_device_t* p_device, u32 offset) {
    const u32 address = PCI_CREATE_CONFIG_ADDRESS(p_device->bus, p_device->device, p_device->function, offset & ~0x3);
    amd64_out_port32(PCI_CONFIG_ADDRESS, address);
    return amd64_in_port32(PCI_CONFIG_DATA);
}

void pci_config_write(const pci_device_t* p_device, u32 offset, u32 value) {
    const u32 address = PCI_CREATE_CONFIG_ADDRESS(p_device->bus, p_device->device, p_device->function, offset & ~0x3);
    amd64_out_port32(PCI_CONFIG_ADDRESS, address);
    amd64_out_port32(PCI_CONFIG_DATA, value);
}

const char* pci_get_class_description(const pci_device_t* p_device) {
    switch (p_device->class_info.class_code) {
        case 0x01: // Mass Storage Controller
            switch (p_device->class_info.sub_class) {
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
            switch (p_device->class_info.sub_class) {
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
            switch (p_device->class_info.sub_class) {
                case 0x03: return "USB Controller";
                default: return "Unknown Serial Bus Controller";
            }
        case 0x0D: // Wireless Controller
            switch (p_device->class_info.sub_class) {
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

u32 pci_read_bar(const pci_device_t* p_device, u32 bar) {
    if (bar > 5)
        return 0;

    return pci_config_read(p_device, PCI_GET_BAR_OFFSET(bar));
}

bool pci_write_bar(const pci_device_t* device, u32 bar, u32 value) {
    if (bar > 5)
        return false;

    pci_config_write(device, PCI_GET_BAR_OFFSET(bar), value);

    return true;
}

bool pci_enumerate_devices(pcie_device_manager_t* device_manager) {
        for (u32 bus = 0; bus < 256; bus++) {
        for (u32 device = 0; device < 32; device++) {
            for (u32 function = 0; function < 8; function++) {
                pci_device_t pci_device {};
                pci_device.bus = bus;
                pci_device.device = device;
                pci_device.function = function;

                pci_device.vendor_device_id.raw = pci_config_read(&pci_device, 0);
                pci_device.class_info.raw = pci_config_read(&pci_device, 8);

                if (pci_device.vendor_device_id.raw == PCI_VENDOR_DEVICE_ID_INVALID)
                    continue;

                device_manager->devices.insert_back(pci_device);
            }
        }
    }

    return true;
}

bool find_device_vendor_device_id(const pci_device_t* p_device, const pci_vendor_device_id_t* p_req) {
    return (p_req->vendor_id == (u16)PCI_UNKNOWN || p_device->vendor_device_id.vendor_id == p_req->vendor_id) &&
           (p_req->device_id == (u16)PCI_UNKNOWN || p_device->vendor_device_id.device_id == p_req->device_id);
}

bool find_device_class_info(const pci_device_t* p_device, const pci_class_info_t* p_req) {
    return (p_req->class_code == (u8)PCI_UNKNOWN || p_device->class_info.class_code == p_req->class_code) &&
           (p_req->sub_class == (u8)PCI_UNKNOWN || p_device->class_info.sub_class == p_req->sub_class) &&
           (p_req->prog_if == (u8)PCI_UNKNOWN || p_device->class_info.prog_if == p_req->prog_if) &&
           (p_req->revision_id == (u8)PCI_UNKNOWN || p_device->class_info.revision_id == p_req->revision_id);
}

pci_device_t* pci_find_device(pcie_device_manager_t* device_manager, const pci_vendor_device_id_t* p_vendor_device_id_target) {
    for (auto& device : device_manager->devices) {
        if (find_device_vendor_device_id(&device, p_vendor_device_id_target))
            return &device;
    }

    return nullptr;
}

pci_device_t* pci_find_device(pcie_device_manager_t* device_manager, const pci_class_info_t* p_class_info_target) {
    for (auto& device : device_manager->devices) {
        if (find_device_class_info(&device, p_class_info_target))
            return &device;
    }

    return nullptr;
}

void pci_loop_devices(pcie_device_manager_t* device_manager, void(*callback)(const pci_device_t*)) {
    if (!device_manager || !callback)
        return;

    for (const auto& device : device_manager->devices)
        callback(&device);
}

bool pci_cmd_enable(const pci_device_t* pcie_device, u32 flags) {
    if (!pcie_device)
        return false;

    u32 cmd = pci_config_read(pcie_device, PCI_COMMAND);
    cmd |= flags;
    pci_config_write(pcie_device, PCI_COMMAND, cmd);

    return true;
}