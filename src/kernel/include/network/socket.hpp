//==========================================
/// @file       socket.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include  "common.hpp"

struct socket_t;
typedef void(*socket_listener_t)(socket_t* socket, u32 ip, u16 port, const u8* data, size_t size);

enum class socket_protocol_t {
    UNKNOWN = 0,
    UDP,
    TCP
};

struct socket_t {
    socket_protocol_t protocol;
    socket_listener_t listener;
    u32 local_ip;
    u16 local_port;

    u32 remote_ip;
    u16 remote_port;

    void* socket_data;
};

bool socket_receive(socket_protocol_t protocol, u16 dst_port, u32 src_ip, u16 src_port, const u8* data, size_t size);
bool socket_send(socket_t* socket, const u8* data, size_t size);
bool socket_bind(socket_t* socket);
socket_t* socket_get(socket_protocol_t protocol, u16 dst_port);

#endif // __SOCKET_HPP__