//==========================================
/// @file       nidm.hpp
/// @brief      network interace driver manager
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_NIDM_HPP__
#define __DRIVERS_NETWORK_NIDM_HPP__

#include "common.hpp"
#include "utils/vector.hpp"
#include "string.hpp"
#include "utils/pointer.hpp"

struct network_interface_device_t {
    string name;
    uint8_t mac[6];
    uint32_t ip4;
    bool is_up;

    int (*send_packet)(const void* data, size_t size);
};

struct nidm_network_packet_t {
    ptr::unique<uint8_t> data;
    size_t size;
};

int nidm_packet_recieve(network_interface_device_t* p_device, const void* p_data, size_t size);
int nidm_send_data(const void* p_data, size_t size);
void nidm_register_device(network_interface_device_t device);
network_interface_device_t* ndim_get_device(const string& name);

#endif // __DRIVERS_NETWORK_NIDM_HPP__