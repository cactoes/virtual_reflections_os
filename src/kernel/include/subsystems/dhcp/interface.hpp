//==========================================
/// @file       interface.hpp
/// @brief      dhcp subsystem interface definition
//==========================================

#pragma once

#ifndef __SUBSYSTEM_DHCP_INTERFACE_HPP__
#define __SUBSYSTEM_DHCP_INTERFACE_HPP__

#include "common.hpp"
#include "drivers/network/nidm.hpp"
#include "subsystem_interface.hpp"

class subsystem_interface_dhcp_client_t : public subsystem_interface_t {
public:
    virtual ~subsystem_interface_dhcp_client_t() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual void configure(network_interface_device_t* device) = 0;
};

#endif // __SUBSYSTEM_DHCP_INTERFACE_HPP__