//==========================================
/// @file       nidm.hpp
/// @brief      network interace driver manager
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_NIDM_HPP__
#define __DRIVERS_NETWORK_NIDM_HPP__

#include "common.hpp"
#include "utils/map.hpp"
#include "string.hpp"
#include "std/pointer.hpp"
#include "std/array.hpp"
#include "utils/mutex.hpp"
#include "std/function.hpp"

union ipv4_address_t {
    uint32_t raw;
    struct {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
    } PACKED;
};

class network_interface_device_t {
public:
    virtual ~network_interface_device_t() = default;

    virtual int send_packet(const void* data, size_t size) = 0;

public:
    string name;
    string interface;
    bool is_up;
    bool is_prefered;
    bool is_configured;
    uint8_t mac[6];

    ipv4_address_t ip;
    ipv4_address_t gateway;
    ipv4_address_t subnet_mask;
};

// typedef void(*network_callback_t)(uint8_t* p_packet, size_t length);
typedef std::function_t<void, uint8_t*, size_t> network_callback_t;

struct nidm_t {
    mutex_t mutex {};

    std::dynamic_array<std::unique_ptr<network_interface_device_t>> devices;
    linear_map<uint16_t, network_callback_t> udp_callbacks;
};

void set_global_nidm(nidm_t* nidm);
nidm_t* get_global_nidm();

void nidm_init(nidm_t* nidm);
void nidm_shutdown(nidm_t* nidm);

int nidm_register_device(nidm_t* nidm, std::unique_ptr<network_interface_device_t> device);
network_interface_device_t* nidm_get_device(nidm_t* nidm, const string& name);

int nidm_packet_recieve(nidm_t* nidm, network_interface_device_t* p_device, const void* p_data, size_t size);
int nidm_packet_send(nidm_t* nidm, const void* p_data, size_t size);

int nidm_udp_bind(nidm_t* nidm, uint16_t port, network_callback_t p_callback);

int nidm_udp_dispatch(nidm_t* nidm, uint16_t port, uint8_t* p_packet, size_t length);

network_interface_device_t* nidm_get_prefered_device(nidm_t* nidm);

#endif // __DRIVERS_NETWORK_NIDM_HPP__