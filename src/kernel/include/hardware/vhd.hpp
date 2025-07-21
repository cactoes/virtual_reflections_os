//==========================================
/// @file       vhd.hpp
/// @brief      virtual hardware driver
///             a virtual system to manage hardware
///             this subsystem is still very subject to change
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

    bool is_ps2_device;
};

harware_device_t* find_device(const char* p_name);
int mount_device(const char* p_name, pci_device_t* p_device, bool is_ps2_device = false);

#endif // __HARWARE_VHD_HPP__}