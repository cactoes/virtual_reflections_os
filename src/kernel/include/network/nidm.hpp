//==========================================
/// @file       nidm.hpp
/// @brief      network interace driver manager
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_NIDM_HPP__
#define __DRIVERS_NETWORK_NIDM_HPP__

#include "common.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/pointer.hpp"
#include "std/array.hpp"
#include "utils/mutex.hpp"
#include "std/function.hpp"
#include "std/ring_buffer.hpp"

union ipv4_address_t {
    uint32_t raw;
    struct {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
    } PACKED;
};

enum class network_interface_device_type_t {
    UNKNOWN = 0,
    E1000
};

struct network_interface_t {
    uint8_t mac[6];

    ipv4_address_t ip;
    ipv4_address_t gateway;
    ipv4_address_t subnet_mask;

    struct {
        bool is_configured : 1;
        bool is_active : 1;
        bool is_prefered : 1;
    } PACKED;

    char device_name[64];

    network_interface_device_type_t device_type;
    void* device;
};

struct network_packet_t {
    uint8_t* data;
    size_t size;
    network_interface_t* interface;
};

struct network_interface_controller_t {
    std::dynamic_array<std::unique_ptr<network_interface_t>> interfaces;
    std::ring_buffer<64, network_packet_t> incoming_packets;
    std::ring_buffer<64, network_packet_t> outgoing_packets;
};

void set_global_nic(network_interface_controller_t* nic);
network_interface_controller_t* get_global_nic();

void nic_init(network_interface_controller_t* nic);

bool nic_register_interface(network_interface_controller_t* nic, std::unique_ptr<network_interface_t> interface);

bool nic_process_packet(network_interface_controller_t* nic);
bool nic_dispatch_packet(network_interface_controller_t* nic, const network_packet_t& packet);
bool nic_receive_packet(network_interface_controller_t* nic, const network_packet_t& packet);

network_interface_t* route_lookup(network_interface_controller_t* nic, uint32_t dst_ip);
network_interface_t* nic_get_default_interface(network_interface_controller_t* nic);
network_interface_t* nic_get_interface_from_device(network_interface_controller_t* nic, void* device);

int nic_thread();

// class network_interface_device_t {
// public:
//     virtual ~network_interface_device_t() = default;

//     virtual int send_packet(const void* data, size_t size) = 0;

// public:
//     std::string name;
//     std::string interface;
//     bool is_up;
//     bool is_prefered;
//     bool is_configured;
//     uint8_t mac[6];

//     ipv4_address_t ip;
//     ipv4_address_t gateway;
//     ipv4_address_t subnet_mask;
// };

// // typedef void(*network_callback_t)(uint8_t* p_packet, size_t length);
// typedef std::function_t<void, uint8_t*, size_t> network_callback_t;

// struct nidm_t {
//     mutex_t mutex {};

//     std::dynamic_array<std::unique_ptr<network_interface_device_t>> devices;
//     std::linear_map<uint16_t, network_callback_t> udp_callbacks;
// };

// void set_global_nidm(nidm_t* nidm);
// nidm_t* get_global_nidm();

// void nidm_init(nidm_t* nidm);
// void nidm_shutdown(nidm_t* nidm);

// int nidm_register_device(nidm_t* nidm, std::unique_ptr<network_interface_device_t> device);
// network_interface_device_t* nidm_get_device(nidm_t* nidm, const std::string& name);
// network_interface_device_t* nidm_get_device_on_interface(nidm_t* nidm, const std::string& interface);

// int nidm_packet_recieve(nidm_t* nidm, network_interface_device_t* p_device, const void* p_data, size_t size);

// // FIXME @since 25/11/2025 -- 20:44
// // temporary fix for the new interupt stuff
// // fix or "harden" this together with `nidm_packet_recieve`
// int nidm_process_packet();

// int nidm_packet_send(nidm_t* nidm, const void* p_data, size_t size);

// int nidm_udp_bind(nidm_t* nidm, uint16_t port, network_callback_t p_callback);

// int nidm_udp_dispatch(nidm_t* nidm, uint16_t port, uint8_t* p_packet, size_t length);

// network_interface_device_t* nidm_get_prefered_device(nidm_t* nidm);

#endif // __DRIVERS_NETWORK_NIDM_HPP__