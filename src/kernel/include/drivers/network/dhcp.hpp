//==========================================
/// @file       dhcp.hpp
/// @brief      dhcp stuff
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_DHCP_HPP__
#define __DRIVERS_NETWORK_DHCP_HPP__

#include "../../../../network_drivers/include/dhcp.hpp"
#include "drivers/network/nidm.hpp"
#include "drivers/driver.hpp"

struct dhcp_context_t {
    DHCPClientState* state;
    system_driver_handle_t driver_handle;
};

dhcp_context_t* dhcp_context_create();
void dhcp_context_destroy(dhcp_context_t* dhcp_context);

void set_global_dhcp_context(dhcp_context_t* dhcp_context);
dhcp_context_t* get_global_dhcp_context();

void dhcp_send_packet(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port, uint8_t* data, size_t size);
void dhcp_client_start(dhcp_context_t* dhcp_context, network_interface_device_t* device);

#endif // __DRIVERS_NETWORK_DHCP_HPP__