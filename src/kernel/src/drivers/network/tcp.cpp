#include "drivers/network/tcp.hpp"
#include "drivers/network/ip.hpp"
#include "utils/vector.hpp"
#include "std/random.hpp"
#include "time/clock.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

linked_list<std::unique_ptr<tcp_connection_t>> connections {};

void tcp_init_connection(tcp_connection_t* connection, uint32_t ip, uint32_t port, tcp_connect_callback_t callback) {
    connection->local_ip = nidm_get_prefered_device(get_global_nidm())->ip.raw;
    connection->local_port = random_number(49152, 65535);
    connection->remote_ip = ip;
    connection->remote_port = port;
    connection->state = tcp_state_t::SYN_SENT;
    connection->iss = (uint32_t)random_number(0, MAX_UINT32 - 1);
    connection->snd_nxt = connection->iss;
    connection->snd_una = connection->iss;
    connection->callback = callback;
}

tcp_connection_t* tcp_connect(uint32_t ip, uint32_t port, tcp_connect_callback_t callback) {
    tcp_connection_t* connection_ptr = nullptr;
    for (auto& c : connections) {
        if (c->state == tcp_state_t::CLOSED) {
            memzero(c.get(), sizeof(tcp_connection_t));
            connection_ptr = c.get();
        }
    }

    if (!connection_ptr) {
        auto connection = std::make_unique<tcp_connection_t>();
        connection_ptr = connection.get();
        connections.insert_back(move(connection));
    }

    tcp_init_connection(connection_ptr, ip, port, callback);
    connection_ptr->state = tcp_state_t::SYN_SENT;
    tcp_send_packet(nullptr, 0, TCP_FLAG_SYN, connection_ptr);
    
    uint64_t time = clock_get_time_since_boot() + 50;
    bool keep_alive = true;
    int retry_count_max = 0;
    while (keep_alive) {
        if (time < clock_get_time_since_boot())
            time = clock_get_time_since_boot() + 100 * (retry_count_max + 1);
        else
            continue;

        switch (connection_ptr->state) {
            case tcp_state_t::ESTABLISHED: {
                keep_alive = false;
                break;
            }
            case tcp_state_t::SYN_SENT: {
                if (retry_count_max > 3) {
                    keep_alive = false;
                    break;
                }
                tcp_send_packet(nullptr, 0, TCP_FLAG_SYN, connection_ptr);
                retry_count_max++;
                break;
            }
            default: {
                keep_alive = false;
                break;
            }
        }
    }

    // give the server some time to respond
    while (time + 50 < clock_get_time_since_boot());
    return connection_ptr;
}

tcp_connection_t* tcp_listen(uint32_t port, tcp_connect_callback_t callback) {
    tcp_connection_t* connection_ptr = nullptr;
    for (auto& c : connections) {
        if (c->state == tcp_state_t::CLOSED) {
            memzero(c.get(), sizeof(tcp_connection_t));
            connection_ptr = c.get();
        }
    }

    if (!connection_ptr) {
        auto connection = std::make_unique<tcp_connection_t>();
        connection_ptr = connection.get();
        connections.insert_back(move(connection));
    }

    connection_ptr->local_ip =  nidm_get_prefered_device(get_global_nidm())->ip.raw;
    connection_ptr->local_port = port;
    connection_ptr->remote_ip = 0;
    connection_ptr->remote_port = 0;
    connection_ptr->state = tcp_state_t::LISTEN;
    connection_ptr->callback = callback;

    return connection_ptr;
}

uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, const tcp_header_t* tcp_header, size_t payload_len) {
    uint32_t sum = 0;
    size_t total_len = sizeof(tcp_header_t) + payload_len;

    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += 6;
    sum += total_len;

    const uint8_t* data = (const uint8_t*)tcp_header;
    size_t len = sizeof(tcp_header_t) + payload_len;

    for (size_t i = 0; i < len - 1; i += 2)
        sum += ((data[i] << 8) + data[i + 1]);

    if (len & 1)
        sum += data[len - 1] << 8;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return htons(~sum);
}

bool tcp_send_packet(uint8_t* p_payload, size_t payload_length, uint8_t flags, tcp_connection_t* connection) {
    // TODO @since 18/09/2025 -- 16:12
    // handle no connection stuff better
    if (!connection)
        return false;

    if (payload_length > 1460) {
        printf(DBG, "[INET - TCP] payload too large (%zu bytes)\n", payload_length);
        return false;
    }

    const size_t tcp_header_len = sizeof(tcp_header_t);
    const size_t total_tcp_len = tcp_header_len + payload_length;
    uint8_t tcp_packet[1480];

    tcp_header_t* tcp_hdr = (tcp_header_t*)tcp_packet;
    memzero(tcp_hdr, sizeof(tcp_header_t));

    tcp_hdr->src_port = host_to_net(connection->local_port);
    tcp_hdr->dst_port = host_to_net(connection->remote_port);
    tcp_hdr->seq_num = host_to_net(connection->snd_nxt);

    if (flags & TCP_FLAG_ACK)
        tcp_hdr->ack_num = host_to_net(connection->rcv_nxt);
    else
        tcp_hdr->ack_num = 0;
    
    if (payload_length > 0)
        connection->snd_nxt += payload_length;
    
    if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN))
        connection->snd_nxt++;

    TCP_SET_DATA_OFFSET(tcp_hdr, tcp_header_len / 4);
    tcp_hdr->flags = flags;
    tcp_hdr->window = host_to_net<uint16_t>(8192);
    tcp_hdr->checksum = 0;
    tcp_hdr->urgent_ptr = 0;
    
    uint8_t* tcp_payload = tcp_packet + tcp_header_len;
    if (p_payload && payload_length > 0)
        memcpy(tcp_payload, p_payload, payload_length);

    auto device = nidm_get_prefered_device(get_global_nidm());

    tcp_hdr->checksum = tcp_checksum(device->ip.raw, connection->remote_ip, tcp_hdr, payload_length);

    return ip_send(device, connection->remote_ip, IP_PROTOCOL_TCP, tcp_packet, total_tcp_len);
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

    // printf(DBG, "[INET - TCP: %s] got tcp packet for from: %u.%u.%u.%u:%u\n", device->name.c_str(), (src_ip >> 24) & 0xff, (src_ip >> 16) & 0xff, (src_ip >> 8) & 0xff, src_ip & 0xff, src_port);

    size_t header_len = data_offset * 4;
    if (header_len > payload_length) {
        printf(DBG, "[INET - TCP: %s] invalid header length\n", device->name.c_str());
        return;
    }

    uint8_t* tcp_data = payload + header_len;
    size_t tcp_data_len = payload_length - header_len;

    tcp_connection_t* connection = nullptr;
    for (auto& con : connections) {
        if (con->local_ip == device->ip.raw &&
            con->local_port == dst_port &&
            con->remote_ip == src_ip &&
            con->remote_port == src_port) {
            connection = con.get();
            break;
        }

        if (con->local_ip == device->ip.raw &&
            con->local_port == dst_port &&
            con->remote_ip == 0 &&
            con->remote_port == 0) {
            connection = con.get();
            connection->remote_ip = src_ip;
            connection->remote_port = src_port;
            break;
        }
    }

    // FIXME @since 23/09/2025 -- 18:21
    // for now if we dont have a specific listener just close the connection
    if (!connection) {
        printf(DBG, "[INET - TCP: %s] no handler\n", device->name.c_str());
        tcp_send_packet(nullptr, 0, TCP_FLAG_RST, nullptr);
        return;
    }

    if (flags & TCP_FLAG_RST) {
        // connection aborted
        connection->state = tcp_state_t::CLOSED;
        return;
    }

    switch (connection->state) {
        case tcp_state_t::LISTEN:
            if (!(flags & TCP_FLAG_SYN))
                break;

            connection->irs = seq_num;
            connection->rcv_nxt = seq_num + 1;
            connection->iss = (uint32_t)random_number(0, MAX_UINT32 - 1);
            connection->snd_nxt = connection->iss;
            connection->state = tcp_state_t::SYN_RECEIVED;
            printf(DBG, "[INET - TCP: %s] incoming connection attempt\n", device->name.c_str());
            if (!tcp_send_packet(nullptr, 0, TCP_FLAG_SYN | TCP_FLAG_ACK, connection))
                connection->state = tcp_state_t::LISTEN;
            break;
        case tcp_state_t::SYN_SENT:
            if (!(flags & TCP_FLAG_SYN) || !(flags & TCP_FLAG_ACK))
                break;
            if (ack_num != connection->snd_nxt)
                break;

            connection->state = tcp_state_t::ESTABLISHED;
            connection->rcv_nxt = seq_num + 1;
            connection->snd_una = ack_num;
            printf(DBG, "[INET - TCP: %s] accepted incoming connection\n", device->name.c_str());
            tcp_send_packet(nullptr, 0, TCP_FLAG_ACK, connection);
            break;
        case tcp_state_t::SYN_RECEIVED:
            if (!(flags & TCP_FLAG_ACK))
                break;

            if (ack_num != connection->snd_nxt) {
                printf(DBG, "[INET - TCP: %s] invalid ACK number, expected %u got %u\n", device->name.c_str(), connection->snd_nxt, ack_num);
                break;
            }

            printf(DBG, "[INET - TCP: %s] connection established!\n", device->name.c_str());
            connection->state = tcp_state_t::ESTABLISHED;
            connection->snd_una = ack_num;
            break;
        case tcp_state_t::ESTABLISHED:
            // TODO @since 23/09/2025 -- 19:58
            // handle PSH
            if (tcp_data_len > 0) {
                if (connection->callback)
                    connection->callback(tcp_data, tcp_data_len);

                connection->rcv_nxt += tcp_data_len;
                tcp_send_packet(nullptr, 0, TCP_FLAG_ACK, connection);
            }

            if (flags & TCP_FLAG_FIN) {
                connection->rcv_nxt++;
                tcp_send_packet(nullptr, 0, TCP_FLAG_FIN | TCP_FLAG_ACK, connection);
                connection->state = tcp_state_t::CLOSE_WAIT;
            }
            break;
        case tcp_state_t::CLOSE_WAIT:
            break;
        case tcp_state_t::LAST_ACK:
            if ((flags & TCP_FLAG_ACK) && ack_num == connection->snd_nxt) {
                connection->state = tcp_state_t::CLOSED;
                // TODO @since 23/09/2025 -- 18:33
                // resource cleanup
            }
        break;
        default:
            printf(DBG, "[INET - TCP: %s] unhandled connection state\n", device->name.c_str());
            break;
    }
}