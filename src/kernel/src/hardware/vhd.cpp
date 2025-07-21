#include "hardware/vhd.hpp"

static vector<harware_device_t> g_hardware_devices {};

harware_device_t* find_device(const char* p_name) {
    for (auto& device : g_hardware_devices) {
        if (device.name == p_name) {
            return &device;
        }
    }

    return nullptr;
}

int mount_device(const char* p_name, pci_device_t* p_device, bool is_ps2_device) {
    if (find_device(p_name))
        return 1;

    harware_device_t hwd {};
    hwd.name = p_name;
    hwd.is_ps2_device = is_ps2_device;
    if (p_device) {
        hwd.has_pci_device = true;
        memcpy(&hwd.pci_device, p_device, sizeof(pci_device_t));
    }
    g_hardware_devices.insert_back(move(hwd));

    return 0;
}