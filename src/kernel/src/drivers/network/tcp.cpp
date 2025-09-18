#include "drivers/network/tcp.hpp"
#include "drivers/network/ip.hpp"
#include "utils/vector.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

tcp_connection_t connections[1] {
    {
        .local_ip = TO_IP(10, 0, 2, 15),
        .local_port = 4321,
        .remote_ip = TO_IP(10, 0, 2, 1),
        .remote_port = 1234,

        .snd_nxt = 0 + 1,
        .snd_una = 0,
        .rcv_nxt = 0 + 1,
        .iss = 0,
        .irs = 0,

        .state = tcp_state_t::LISTEN,
    }
};

void tcp_send(network_interface_device_t* p_device, uint8_t* p_payload, size_t payload_length, uint8_t flags, tcp_connection_t* connection) {
    // TODO @since 18/09/2025 -- 16:12
    // handle no connection stuff better
    if (!connection)
        return;

    if (payload_length > 1460) {
        printf(DBG, "[INET - TCP: %s] payload too large (%zu bytes)\n", p_device->name.c_str(), payload_length);
        return;
    }

    const size_t tcp_header_len = sizeof(tcp_header_t);
    const size_t total_tcp_len = tcp_header_len + payload_length;
    uint8_t tcp_packet[1480];

    tcp_header_t* tcp_hdr = (tcp_header_t*)tcp_packet;
    memset(tcp_hdr, 0, sizeof(tcp_header_t));

    tcp_hdr->src_port = host_to_net(connection->local_port);
    tcp_hdr->dst_port = host_to_net(connection->remote_port);
    tcp_hdr->seq_num = host_to_net(connection->snd_nxt);

    if (flags & TCP_FLAG_ACK) {
        tcp_hdr->ack_num = host_to_net(connection->rcv_nxt);
    } else {
        tcp_hdr->ack_num = 0;
    }
    
    if (payload_length > 0) {
        connection->snd_nxt += payload_length;
    }
    
    if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)) {
        connection->snd_nxt++;
    }

    TCP_SET_DATA_OFFSET(tcp_hdr, tcp_header_len / 4);
    tcp_hdr->flags = flags;
    tcp_hdr->window = host_to_net<uint16_t>(8192);
    tcp_hdr->checksum = 0;
    tcp_hdr->urgent_ptr = 0;
    
    uint8_t* tcp_payload = tcp_packet + tcp_header_len;
    if (p_payload && payload_length > 0)
        memcpy(tcp_payload, p_payload, payload_length);

    ip_send(p_device, connection->remote_ip, IP_PROTOCOL_TCP, tcp_packet, total_tcp_len);
}

void tcp_receive(network_interface_device_t* device, uint8_t* payload, size_t payload_length, uint32_t src_ip) {
    if (payload_length < sizeof(tcp_header_t)) {
        printf(DBG, "[INET - TCP: %s] packet too small\n", device->name.c_str());
        return;
    }

    tcp_header_t* header = (tcp_header_t*)payload;

    uint16_t src_port = net_to_host(header->src_port);
    uint16_t dst_port = net_to_host(header->dst_port);
    uint32_t seq_num = net_to_host(header->seq_num);
    uint32_t ack_num = net_to_host(header->ack_num);
    uint8_t data_offset = TCP_DATA_OFFSET(header);
    uint8_t flags = header->flags;
    uint16_t window = net_to_host(header->window);

    printf(DBG, "[INET - TCP: %s] got tcp packet for from: %u.%u.%u.%u:%u\n", device->name.c_str(), (src_ip >> 24) & 0xff, (src_ip >> 16) & 0xff, (src_ip >> 8) & 0xff, src_ip & 0xff, dst_port);

    size_t header_len = data_offset * 4;
    if (header_len > payload_length) {
        printf(DBG, "[INET - TCP: %s] invalid header length\n", device->name.c_str());
        return;
    }

    uint8_t* tcp_data = payload + header_len;
    size_t tcp_data_len = payload_length - header_len;

    tcp_connection_t* connection = nullptr;
    for (auto& con : connections) {
        if (con.local_ip == device->ip4 &&
            con.local_port == dst_port &&
            con.remote_ip == src_ip &&
            con.remote_port == src_port)
            connection = &con;
    }

    if (flags & TCP_FLAG_SYN) {
        if (!connection) {
            // reject since we dont listen on this port
            printf(DBG, "[INET - TCP: %s] no handler\n", device->name.c_str());
            tcp_send(device, nullptr, 0, TCP_FLAG_RST | TCP_FLAG_ACK, nullptr);
            return;
        }

        connection->state = tcp_state_t::SYN_RECEIVED;
        printf(DBG, "[INET - TCP: %s] new SYN request\n", device->name.c_str());
        // TODO @since 18/09/2025 -- 21:36
        // send proper syn-ack??
        tcp_send(device, nullptr, 0, TCP_FLAG_SYN | TCP_FLAG_ACK, connection);
        return;
    }

    if (!connection) {
        // reject since we dont have a connection and its not attempting to connect
        printf(DBG, "[INET - TCP: %s] no handler\n", device->name.c_str());
        tcp_send(device, nullptr, 0, TCP_FLAG_RST, nullptr);
        return;
    }

    printf(DBG, "[INET - TCP: %s] new unhandled packet\n", device->name.c_str());


    if (flags & TCP_FLAG_ACK) {
        printf(DBG, "[INET - TCP: %s] new ACK\n", device->name.c_str());
    }
}