#include "network/socket.hpp"
#include "std/array.hpp"

static std::dynamic_array<socket_t*> global_sockets {};

bool send_socket(socket_protocol_t protocol, uint16_t port, const uint8_t* data, size_t size) {
    for (auto& s : global_sockets)
        if (s->port == port && s->protocol == protocol)
            s->listener(data, size);

    return false;
}

void socket_bind(socket_t* socket) {
    for (auto& s : global_sockets)
        if (socket->port == s->port && socket->protocol == s->protocol)
            return;

    global_sockets.insert_back(socket);
}