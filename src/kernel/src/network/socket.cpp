#include "network/socket.hpp"
#include "network/udp.hpp"
#include "network/tcp.hpp"
#include "std/array.hpp"
#include "virtual_thread.hpp"
#include "io.hpp"

static std::dynamic_array<socket_t*> global_sockets {};

bool socket_receive(socket_protocol_t protocol, u16 dst_port, u32 src_ip, u16 src_port, const u8* data, size_t size) {
    u32 dst_port2 = (u32)dst_port;
    
    for (auto s : global_sockets) {
        if (s->local_port == dst_port && s->protocol == protocol) {
            printf("[ socket ] socket destination -> [port: %u] [protocol: %u]\n", dst_port2, (u32)protocol);
            s->listener(s, src_ip, src_port, data, size);
            return true;
        }
    }

    printf("[ socket ] no destination for: [port: %u] [protocol: %u]\n", dst_port2, (u32)protocol);
    return false;
}

bool socket_bind(socket_t* socket) {
    for (auto& s : global_sockets)
        if (socket->local_port == s->local_port && socket->protocol == s->protocol)
            return false;

    global_sockets.insert_back(socket);
    return true;
}

bool socket_send(socket_t* socket, const u8* data, size_t size) {
    if (!socket || !data)
        return false;

    switch (socket->protocol) {
        case socket_protocol_t::UDP:
            return udp_send(socket->remote_ip, socket->local_port, socket->remote_port, data, size);
        case socket_protocol_t::TCP: {
            if (!socket->socket_data) {
                socket->socket_data = (tcb_t*)tcp_create_tcb(socket->local_ip, socket->local_port, socket->remote_ip, socket->remote_port);
                if (!tcp_send((tcb_t*)socket->socket_data, nullptr, 0))
                    return false;
            }

            while (!tcp_is_connection_established((tcb_t*)socket->socket_data))
                vthread_sleep(1);

            return tcp_send((tcb_t*)socket->socket_data, data, size);
        }
        default:
            break;
    }

    return false;
}

socket_t* socket_get(socket_protocol_t protocol, u16 dst_port) {
    for (auto s : global_sockets) {
        if (s->local_port == dst_port && s->protocol == protocol) {
            return s;
        }
    }

    return nullptr;
}