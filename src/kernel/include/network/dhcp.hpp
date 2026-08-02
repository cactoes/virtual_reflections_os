//==========================================
/// @file       dhcp.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DHCP_HPP__
#define __DHCP_HPP__

#define DHCP_PORT_CLIENT                        68
#define DHCP_PORT_SERVER                        67

#define DHCP_MAGIC                              (u32)0x63825363

#define DHCP_OP_BOOTREQUEST                     1
#define DHCP_OP_BOOTREPLY                       2

#define DHCP_HTYPE_ETHERNET                     1
#define DHCP_HLEN_ETHERNET_ADDRESS              6

#define DHCP_MESSAGE_TYPE_DHCPDISCOVER          1
#define DHCP_MESSAGE_TYPE_DHCPOFFER             2
#define DHCP_MESSAGE_TYPE_DHCPREQUEST           3
#define DHCP_MESSAGE_TYPE_DHCPDECLINE           4
#define DHCP_MESSAGE_TYPE_DHCPACK               5
#define DHCP_MESSAGE_TYPE_DHCPNAK               6
#define DHCP_MESSAGE_TYPE_DHCPRELEASE           7
#define DHCP_MESSAGE_TYPE_DHCPINFORM            8
#define DHCP_MESSAGE_TYPE_DHCPFORCERENEW        9
#define DHCP_MESSAGE_TYPE_DHCPLEASEQUERY        10
#define DHCP_MESSAGE_TYPE_DHCPLEASEUNASSIGNED   11
#define DHCP_MESSAGE_TYPE_DHCPLEASEUNKNOWN      12
#define DHCP_MESSAGE_TYPE_DHCPLEASEACTIVE       13

#define DHCP_OPTION_PAD                         0
#define DHCP_OPTION_SUBNET_MASK                 1
#define DHCP_OPTION_ROUTER                      3
#define DHCP_OPTION_DNS                         6
#define DHCP_OPTION_HOSTNAME                    12
#define DHCP_OPTION_REQUESTED_IP_ADDR           50
#define DHCP_OPTION_IP_LEASE_TIME               51
#define DHCP_OPTION_DHCP_MESSAGE_TYPE           53
#define DHCP_OPTION_DHCP_SERVER_ID              54
#define DHCP_OPTION_PARAMETER_REQUEST_LIST      55
#define DHCP_OPTION_CLIENT_ID                   61
#define DHCP_OPTION_END                         255

#define DHCP_CLIENT_RECIEVE_ERR                 1
#define DHCP_CLIENT_RECIEVE_ACK                 2
#define DHCP_CLIENT_RECIEVE_OTH                 3
#define DHCP_CLIENT_RECIEVE_REQ                 4

#include "common.hpp"
#include "utils/mutex.hpp"
#include "std/ring_buffer.hpp"
#include "network/nidm.hpp"

struct dhcp_packet_t {
    u8 op;
    u8 htype;
    u8 hlen;
    u8 hops;
    u32 xid;
    u16 secs;
    u16 flags;

    u32 client_ip_addr;
    u32 your_ip_addr;
    u32 server_ip_addr;
    u32 gateway_ip_addr;

    u8 client_hw_addr[16];
    u8 server_name[64];
    u8 file[128];

    u32 magic;

    u8 options[308];
} __packed;

struct dhcp_options_writer_t {
    u8* buffer;
    size_t buffer_size;
    size_t offset;
};

template <size_t size>
struct dhcp_option_t {
    u8 type;
    u8 length;
    u8 value[size];
};

struct dhcp_client_t {
    mutex_t mutex;

    struct session_t {
        u32 xid;
        ipv4_address_t ip;
        ipv4_address_t dhcp_ip;
        u32 lease_time;
        network_interface_t* interface;
    } session;
};

u8* dhcp_option_get(dhcp_packet_t* packet, u8 type);
u32 dhcp_field_to_number(u8* field, size_t size);

dhcp_packet_t dhcp_create_discover_packet(const char* hostname, dhcp_client_t::session_t* session);
dhcp_packet_t dhcp_create_request_packet(const char* hostname, dhcp_client_t::session_t* session, u32 wanted_ip);
dhcp_packet_t dhcp_create_lease_extend_packet(const char* hostname, dhcp_client_t::session_t* session, u32 ip_to_extend);

dhcp_client_t* dhcp_client_create();
dhcp_client_t::session_t dhcp_client_create_session(network_interface_t* target_interface);
bool dhcp_client_destroy(dhcp_client_t* client);

#endif // __DHCP_HPP__