//==========================================
/// @file       network_manager.cpp
/// @brief      kernel network manager
//==========================================

#pragma once

#ifndef __NETWORK_MANAGER_HPP__
#define __NETWORK_MANAGER_HPP__

#define DNS_MAX_WAIT 1000 // miliseconds
#define DEVICE_HOST_NAME "VirtualReflections-Machine"
#define DEFAULT_DNS_SERVER TO_IP(1, 1, 1, 1)

#include "common.hpp"
#include "network/socket.hpp"
#include "network/dhcp.hpp"
#include "network/dns.hpp"
#include "network/nidm.hpp"

struct network_manager_t {
    socket_t* dhcp_socket;
    dhcp_client_t* dhcp_client;

    socket_t* dns_socket;
    dns_client_t* dns_client;
};

network_manager_t* get_global_network_manager();
void set_global_network_manager(network_manager_t* network_manager);

// BUG @since 09/05/2026 -- 20:42
// this function never has a cleanup version so sockets etc are leaked
bool network_manager_init(network_manager_t* network_manager);

bool network_manager_configre_interface(network_manager_t* network_manager, network_interface_t* interface);
u32 network_manager_dns_query(network_manager_t* network_manager, const char* hostname);

bool network_manager_configure(network_manager_t* network_manager);

#endif // __NETWORK_MANAGER_HPP__