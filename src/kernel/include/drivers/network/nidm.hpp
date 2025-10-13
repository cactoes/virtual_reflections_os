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
#include "utils/map.hpp"
#include "string.hpp"
#include "std/pointer.hpp"

struct network_interface_device_t {
    string name;
    bool is_up;

    uint8_t mac[6];
    
    union {
        uint32_t ip4;
        struct {
            uint8_t ipv4_0;
            uint8_t ipv4_1;
            uint8_t ipv4_2;
            uint8_t ipv4_3;
        } PACKED;
    };
    
    union {
        uint32_t gateway_ip;
        struct {
            uint8_t gateway_ip_0;
            uint8_t gateway_ip_1;
            uint8_t gateway_ip_2;
            uint8_t gateway_ip_3;
        } PACKED;
    };

    union {
        uint32_t subnet_mask;
        struct {
            uint8_t subnet_mask_0;
            uint8_t subnet_mask_1;
            uint8_t subnet_mask_2;
            uint8_t subnet_mask_3;
        } PACKED;
    };
    
    void* device_data;

    int (*send_packet)(network_interface_device_t* p_nid, const void* data, size_t size);
};

typedef void(*network_callback_t)(uint8_t* p_packet, size_t length);

struct nidm_t {
    dynamic_array<network_interface_device_t> devices;
    linear_map<uint16_t, network_callback_t> udp_callbacks;
};

void set_global_nidm(nidm_t* nidm);
nidm_t* get_global_nidm();

void nidm_init(nidm_t* nidm);
void nidm_shutdown(nidm_t* nidm);

int nidm_register_device(nidm_t* nidm, const network_interface_device_t& device);
network_interface_device_t* nidm_get_device(nidm_t* nidm, const string& name);

int nidm_packet_recieve(nidm_t* nidm, network_interface_device_t* p_device, const void* p_data, size_t size);
int nidm_packet_send(nidm_t* nidm, network_interface_device_t* p_device, const void* p_data, size_t size);

int nidm_udp_bind(nidm_t* nidm, uint16_t port, network_callback_t p_callback);
int nidm_udp_dispatch(nidm_t* nidm, uint16_t port, uint8_t* p_packet, size_t length);

#endif // __DRIVERS_NETWORK_NIDM_HPP__