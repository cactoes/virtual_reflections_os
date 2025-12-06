//==========================================
/// @file       interface.hpp
/// @brief      dns subsystem interface definition
//==========================================

#pragma once

#ifndef __SUBSYSTEM_DNS_INTERFACE_HPP__
#define __SUBSYSTEM_DNS_INTERFACE_HPP__

#include "common.hpp"
#include "drivers/network/nidm.hpp"
#include "subsystem_interface.hpp"

class subsys_dns_client_t : public subsystem_interface_t {
public:
    virtual ~subsys_dns_client_t() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual uint32_t resolve(const char* hostname) = 0;
    virtual bool is_configured() = 0;
    virtual uint32_t get_dns_server() = 0;
};

#endif // __SUBSYSTEM_DNS_INTERFACE_HPP__