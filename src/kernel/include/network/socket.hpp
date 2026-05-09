//==========================================
/// @file       socket.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include  "common.hpp"

struct socket_t;
typedef void(*socket_listener_t)(socket_t* socket, uint32_t ip, uint16_t port, const uint8_t* data, size_t size);

enum class socket_protocol_t {
    UNKNOWN = 0,
    UDP,
    TCP
};

struct socket_t {
    socket_protocol_t protocol;
    socket_listener_t listener;
    uint32_t local_ip;
    uint16_t local_port;

    uint32_t remote_ip;
    uint16_t remote_port;

    void* socket_data;
};

bool socket_receive(socket_protocol_t protocol, uint16_t dst_port, uint32_t src_ip, uint16_t src_port, const uint8_t* data, size_t size);
bool socket_send(socket_t* socket, const uint8_t* data, size_t size);
bool socket_bind(socket_t* socket);
socket_t* socket_get(socket_protocol_t protocol, uint16_t dst_port);

#endif // __SOCKET_HPP__