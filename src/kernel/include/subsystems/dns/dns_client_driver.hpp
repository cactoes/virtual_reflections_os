//==========================================
/// @file       dns_driver.hpp
/// @brief      driver implementation for the dns subsystem
//==========================================

#pragma once

#ifndef __SUBSYSTEM_DNS_CLIENT_DRIVER_HPP__
#define __SUBSYSTEM_DNS_CLIENT_DRIVER_HPP__

#include "../../../../network_drivers/include/dns.hpp"
#include "subsystems/dns/interface.hpp"
#include "utils/mutex.hpp"

class subsystem_dns_client_driver_t : public subsystem_interface_dns_client_t {
public:
    subsystem_dns_client_driver_t() {
        mutex_init(&mutex);
    };

    bool init() override;
    void shutdown() override;

    uint32_t resolve(const char* hostname) override;
    bool is_configured() override;
    uint32_t get_dns_server() override;

private:
    void network_callback(uint8_t* packet, size_t size);

    DNSClientState* state;
    mutex_t mutex;

    decltype(DNSClientInit)* driver_client_init = nullptr;
    decltype(DNSClientShutdown)* driver_client_shutdown = nullptr;
    decltype(DNSClientHandlePacket)* driver_client_handle_packet = nullptr;
    decltype(DNSClientResolve)* driver_client_resolve = nullptr;
    decltype(DNSClientGetRecord)* driver_client_get_record = nullptr;
};

#endif // __SUBSYSTEM_DNS_CLIENT_DRIVER_HPP__