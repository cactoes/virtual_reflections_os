//==========================================
/// @file       vhd.hpp
/// @brief      virtual hardware driver
///             a virtual system to manage hardware
//==========================================

#pragma once

#ifndef __HARWARE_VHD_HPP__
#define __HARWARE_VHD_HPP__

#include "common.hpp"
#include "string.hpp"
#include "utils/vector.hpp"
#include "drivers/pcie.hpp"

struct harware_device_t {
    pci_device_t pci_device;
    bool has_pci_device;
    string name;
};

inline vector<harware_device_t> g_hardware_devices {};

inline harware_device_t* find_device(const char* name) {

}

inline int mount_device(const char* name, pci_device_t* device) {
    if (find_device(name))
        return 1;

    harware_device_t hwd {};
    g_hardware_devices.insert_back(hwd);

    return 0;
}

#endif // __HARWARE_VHD_HPP__