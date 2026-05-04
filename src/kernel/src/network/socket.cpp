#include "network/socket.hpp"
#include "network/udp.hpp"
#include "network/tcp.hpp"
#include "std/array.hpp"

static std::dynamic_array<socket_t*> global_sockets {};

bool socket_receive(socket_protocol_t protocol, uint16_t dst_port, uint32_t src_ip, uint16_t src_port, const uint8_t* data, size_t size) {
    for (auto s : global_sockets) {
        if (s->port == dst_port && s->protocol == protocol) {
            s->listener(s, src_ip, src_port, data, size);
        }
    }

    return false;
}

void socket_bind(socket_t* socket) {
    for (auto& s : global_sockets)
        if (socket->port == s->port && socket->protocol == s->protocol)
            return;

    global_sockets.insert_back(socket);
}

bool socket_send(socket_t* socket, uint32_t ip, uint16_t port, const uint8_t* data, size_t size) {
    if (!socket || !data)
        return false;

    switch (socket->protocol) {
        case socket_protocol_t::UDP:
            return udp_send(ip, socket->port, port, data, size);
        case socket_protocol_t::TCP: {
            tcb_t* tcb = (tcb_t*)socket->socket_data;
            // TODO @since 04/05/2026 -- 20:05
            // tcp state machine
            return tcp_send(ip, socket->port, port, data, size);
        }
        default:
            break;
    }

    return false;
}