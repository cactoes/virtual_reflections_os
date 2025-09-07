//==========================================
/// @file       nidm.hpp
/// @brief      network interace driver manager
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_NIDM_HPP__
#define __DRIVERS_NETWORK_NIDM_HPP__

#define TO_IP(a0, a1, a2, a3) ((((uint32_t)(a0) & 0xff) << 24) | (((uint32_t)(a1) & 0xff) << 16) | (((uint32_t)(a2) & 0xff) << 8) | (((uint32_t)(a3) & 0xff) << 0))

#include "common.hpp"
#include "utils/vector.hpp"
#include "string.hpp"
#include "utils/pointer.hpp"

struct network_interface_device_t {
    string name;
    bool is_up;

    uint8_t mac[6];
    uint32_t ip4;
    
    void* device_data;

    int (*send_packet)(network_interface_device_t* p_nid, const void* data, size_t size);
};

typedef void(*network_callback_t)(uint8_t* p_packet, size_t length);

int nidm_packet_recieve(network_interface_device_t* p_device, const void* p_data, size_t size);
int nidm_send_data(const string& name, const void* p_data, size_t size);
int nidm_send_data(network_interface_device_t* p_device, const void* p_data, size_t size);
void nidm_register_device(network_interface_device_t device);
int nidm_udp_bind(uint64_t port, network_callback_t p_callback);
int ndim_udp_send_to_handler(uint64_t port, uint8_t* p_packet, size_t length);
network_interface_device_t* ndim_get_device(const string& name);

#endif // __DRIVERS_NETWORK_NIDM_HPP__