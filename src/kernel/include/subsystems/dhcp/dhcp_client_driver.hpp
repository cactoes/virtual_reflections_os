//==========================================
/// @file       dhcp_driver.hpp
/// @brief      driver implementation for the dhcp subsystem
//==========================================

#pragma once

#ifndef __SUBSYSTEM_DHCP_DHCP_DRIVER_HPP__
#define __SUBSYSTEM_DHCP_DHCP_DRIVER_HPP__

#include "../../../../network_drivers/include/dhcp.hpp"
#include "subsystems/dhcp/interface.hpp"
#include "utils/mutex.hpp"

class subsys_dhcp_client_driver_t : public subsys_dhcp_client_t {
public:
    subsys_dhcp_client_driver_t(const std::string& hostname) : hostname(hostname) {
        mutex_init(&mutex);
    };

    bool init() override;
    void shutdown() override;

    void configure(network_interface_device_t* device) override;

private:
    void network_callback(uint8_t* packet, size_t size);

    DHCPClientState* state;
    network_interface_device_t* current_device;
    std::string hostname;
    mutex_t mutex;

    decltype(DHCPClientInit)* driver_client_init = nullptr;
    decltype(DHCPClientShutdown)* driver_client_shutdown = nullptr;
    decltype(DHCPClientStart)* driver_client_start = nullptr;
    decltype(DHCPClientHandlePacket)* driver_client_handle_packet = nullptr;
    decltype(DHCPClientLeaseExtend)* driver_client_lease_extend = nullptr;
};

#endif // __SUBSYSTEM_DHCP_DHCP_DRIVER_HPP__