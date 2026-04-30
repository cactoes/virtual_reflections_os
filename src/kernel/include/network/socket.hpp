//==========================================
/// @file       socket.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __SOCKET_HPP__
#define __SOCKET_HPP__

#include  "common.hpp"

typedef void(*socket_listener_t)(const uint8_t* data, size_t size);

enum class socket_protocol_t {
    UNKNOWN = 0,
    UDP,
    TCP
};

struct socket_t {
    uint16_t port;
    socket_protocol_t protocol;

    socket_listener_t listener;
};

bool send_socket(socket_protocol_t protocol, uint16_t port, const uint8_t* data, size_t size);
void socket_bind(socket_t* socket);

#endif // __SOCKET_HPP__