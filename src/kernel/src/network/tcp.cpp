#include "network/tcp.hpp"
#include "network/ip.hpp"
// #include "utils/vector.hpp"
#include "std/random.hpp"
#include "network/socket.hpp"
// #include "time/clock.hpp"
#include "io.hpp"

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

    return ~sum;
}

uint32_t tcp_generate_isn() {
    return (uint32_t)random_number(0, MAX_UINT32);
}

bool tcp_send_with_flags(tcb_t* tcb, uint32_t flags, const uint8_t* payload, size_t size) {
    const size_t packet_size = sizeof(tcp_header_t) + size;
    uint8_t* packet = (uint8_t*)malloc(packet_size);

    if ((flags & (TCP_FLAG_SYN | TCP_FLAG_FIN | TCP_FLAG_RST | TCP_FLAG_ACK)) == 0) {
        if (!payload || size == 0)
            return false;
    }

    tcp_header_t* header = (tcp_header_t*)packet;
    header->src_port = bswap16(tcb->local_port);
    header->dst_port = bswap16(tcb->remote_port);
    header->seq_num = bswap32(tcb->snd_nxt);

    if (flags & TCP_FLAG_ACK)
        header->ack_num = bswap32(tcb->rcv_nxt);
    else
        header->ack_num = 0;

    TCP_SET_DATA_OFFSET(header, sizeof(tcp_header_t) / 4);
    header->flags = flags;
    header->window = bswap16(4096);
    header->checksum = 0;
    header->urgent_ptr = 0;
    
    uint8_t* tcp_payload = packet + sizeof(tcp_header_t);
    if (payload && size > 0)
        memcpy(tcp_payload, payload, size);

    network_interface_t* interface = route_lookup(get_global_nic(), tcb->remote_ip);

    if (!interface) {
        free(packet);
        return false;
    }

    header->checksum = bswap16(tcp_checksum(
        interface->ip.raw,
        tcb->remote_ip,
        header,
        size
    ));

    bool result = ip_send(interface, tcb->remote_ip, IP_PROTOCOL_TCP, packet, packet_size);
    if (result) {
        if (size > 0)
            tcb->snd_nxt += size;
        
        if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN))
            tcb->snd_nxt++;
    }
    return result;
    
}

bool tcp_send(tcb_t* tcb, const uint8_t* payload, size_t size) {
    if (!tcb)
        return false;

    switch (tcb->state) {
        case tcp_state_t::OPENED:
            tcb->state = tcp_state_t::SYN_SENT;
            return tcp_send_with_flags(tcb, TCP_FLAG_SYN, nullptr, 0);
        case tcp_state_t::SYN_SENT:
            return false;
        case tcp_state_t::ESTABLISHED:
            return tcp_send_with_flags(tcb, TCP_FLAG_ACK | TCP_FLAG_PSH, payload, size);
        default:
            return false;
    }
}

tcb_t* tcp_create_tcb(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip, uint16_t remote_port) {
    tcb_t* tcb = new tcb_t {};
    if (!tcb)
        return nullptr;

    tcb->local_ip = local_ip;
    tcb->local_port = local_port;
    tcb->remote_ip = remote_ip;
    tcb->remote_port = remote_port;

    tcb->snd_nxt = tcp_generate_isn();
    tcb->snd_una = tcb->snd_nxt;
    tcb->rcv_nxt = 0;

    tcb->state = tcp_state_t::OPENED;

    return tcb;
}

void tcp_receive(network_interface_t* interface, uint32_t src_ip, uint8_t* payload, size_t payload_length) {
    tcp_header_t* header = (tcp_header_t*)payload;

    uint16_t src_port = bswap16(header->src_port);
    uint16_t dst_port = bswap16(header->dst_port);
    uint32_t seq_num = bswap32(header->seq_num);
    uint32_t ack_num = bswap32(header->ack_num);
    uint8_t data_offset = TCP_DATA_OFFSET(header);
    uint8_t flags = header->flags;
    uint16_t window = bswap16(header->window);

    socket_t* socket = socket_get(socket_protocol_t::TCP, dst_port);
    if (!socket)
        return;

    tcb_t* tcb = (tcb_t*)socket->socket_data;
    if (!tcb)
        return;

    if (flags & TCP_FLAG_FIN) {
        tcb->rcv_nxt++;
        tcp_send_with_flags(tcb, TCP_FLAG_ACK, nullptr, 0);
        tcb->state = tcp_state_t::CLOSED;
        delete tcb;
        socket->socket_data = nullptr;
        return;
    }

    if (flags & TCP_FLAG_RST) {
        if (seq_num < tcb->rcv_nxt || seq_num >= tcb->rcv_nxt + window)
            return;
        tcb->state = tcp_state_t::CLOSED;
        delete tcb;
        socket->socket_data = nullptr;
        return;
    }

    if (tcb->state == tcp_state_t::SYN_SENT) {
        if (!(flags & TCP_FLAG_SYN) || !(flags & TCP_FLAG_ACK))
            return;
        if (ack_num != tcb->snd_nxt)
            return;

        tcb->rcv_nxt = seq_num + 1;
        tcb->snd_una = ack_num;
        kprintf("[INET - TCP] accepted incoming connection\n");
        tcp_send_with_flags(tcb, TCP_FLAG_ACK, nullptr, 0);
        tcb->state = tcp_state_t::ESTABLISHED;
        return;
    }

    if (tcb->state == tcp_state_t::ESTABLISHED) {
        if (flags & TCP_FLAG_FIN) {
            tcb->state = tcp_state_t::CLOSED;
            tcb->rcv_nxt++;
            tcp_send_with_flags(tcb, TCP_FLAG_FIN | TCP_FLAG_ACK, nullptr, 0);
            return;
        }

        size_t header_len = data_offset * 4;
        uint8_t* tcp_data = payload + header_len;
        size_t tcp_data_len = payload_length - header_len;

        if (tcp_data_len > 0) {
            if (seq_num != tcb->rcv_nxt) {
                // drop out of order packets
                tcp_send_with_flags(tcb, TCP_FLAG_ACK, nullptr, 0);
                return;
            }

            tcb->rcv_nxt += tcp_data_len;
            socket_receive(socket_protocol_t::TCP, dst_port, src_ip, src_port, tcp_data, tcp_data_len);
            tcp_send_with_flags(tcb, TCP_FLAG_ACK, nullptr, 0);
            return;
        }
    }
}

bool tcp_is_connection_established(tcb_t* tcb) {
    return tcb->state == tcp_state_t::ESTABLISHED;
}

// linked_list<std::unique_ptr<tcp_connection_t>> connections {};

// void tcp_init_connection(tcp_connection_t* connection, uint32_t ip, uint32_t port, tcp_connect_callback_t callback) {
//     connection->local_ip = nidm_get_prefered_device(get_global_nidm())->ip.raw;
//     connection->local_port = random_number(49152, 65535);
//     connection->remote_ip = ip;
//     connection->remote_port = port;
//     connection->state = tcp_state_t::SYN_SENT;
//     connection->iss = (uint32_t)random_number(0, MAX_UINT32 - 1);
//     connection->snd_nxt = connection->iss;
//     connection->snd_una = connection->iss;
//     connection->callback = callback;
// }

// tcp_connection_t* tcp_connect(uint32_t ip, uint32_t port, tcp_connect_callback_t callback) {
//     tcp_connection_t* connection_ptr = nullptr;
//     for (auto& c : connections) {
//         if (c->state == tcp_state_t::CLOSED) {
//             memzero(c.get(), sizeof(tcp_connection_t));
//             connection_ptr = c.get();
//         }
//     }

//     if (!connection_ptr) {
//         auto connection = std::make_unique<tcp_connection_t>();
//         connection_ptr = connection.get();
//         connections.insert_back(move(connection));
//     }

//     tcp_init_connection(connection_ptr, ip, port, callback);
//     connection_ptr->state = tcp_state_t::SYN_SENT;
//     tcp_send_packet(nullptr, 0, TCP_FLAG_SYN, connection_ptr);
    
//     uint64_t time = clock_get_time_since_boot() + 400;
//     bool keep_alive = true;
//     int retry_count_max = 0;
//     while (keep_alive) {
//         if (time < clock_get_time_since_boot())
//             time = clock_get_time_since_boot() + 400 + 100 * (retry_count_max + 1);
//         else
//             continue;

//         switch (connection_ptr->state) {
//             case tcp_state_t::ESTABLISHED: {
//                 keep_alive = false;
//                 break;
//             }
//             case tcp_state_t::SYN_SENT: {
//                 if (retry_count_max > 3) {
//                     keep_alive = false;
//                     break;
//                 }
//                 tcp_send_packet(nullptr, 0, TCP_FLAG_SYN, connection_ptr);
//                 retry_count_max++;
//                 break;
//             }
//             default: {
//                 keep_alive = false;
//                 break;
//             }
//         }
//     }

//     // give the server some time to respond
//     while (time + 50 < clock_get_time_since_boot());
//     return connection_ptr;
// }

// tcp_connection_t* tcp_listen(uint32_t port, tcp_connect_callback_t callback) {
//     tcp_connection_t* connection_ptr = nullptr;
//     for (auto& c : connections) {
//         if (c->state == tcp_state_t::CLOSED) {
//             memzero(c.get(), sizeof(tcp_connection_t));
//             connection_ptr = c.get();
//         }
//     }

//     if (!connection_ptr) {
//         auto connection = std::make_unique<tcp_connection_t>();
//         connection_ptr = connection.get();
//         connections.insert_back(move(connection));
//     }

//     connection_ptr->local_ip =  nidm_get_prefered_device(get_global_nidm())->ip.raw;
//     connection_ptr->local_port = port;
//     connection_ptr->remote_ip = 0;
//     connection_ptr->remote_port = 0;
//     connection_ptr->state = tcp_state_t::LISTEN;
//     connection_ptr->callback = callback;

//     return connection_ptr;
// }

// bool tcp_send_packet(uint8_t* p_payload, size_t payload_length, uint8_t flags, tcp_connection_t* connection) {
//     // TODO @since 18/09/2025 -- 16:12
//     // handle no connection stuff better
//     if (!connection)
//         return false;

//     if (payload_length > 1460) {
//         kprintf("[INET - TCP] payload too large (%zu bytes)\n", payload_length);
//         return false;
//     }

//     const size_t tcp_header_len = sizeof(tcp_header_t);
//     const size_t total_tcp_len = tcp_header_len + payload_length;
//     uint8_t tcp_packet[1480];

//     tcp_header_t* tcp_hdr = (tcp_header_t*)tcp_packet;
//     memzero(tcp_hdr, sizeof(tcp_header_t));

//     tcp_hdr->src_port = bswap16(connection->local_port);
//     tcp_hdr->dst_port = bswap16(connection->remote_port);
//     tcp_hdr->seq_num = bswap32(connection->snd_nxt);

//     if (flags & TCP_FLAG_ACK)
//         tcp_hdr->ack_num = bswap32(connection->rcv_nxt);
//     else
//         tcp_hdr->ack_num = 0;
    
//     if (payload_length > 0)
//         connection->snd_nxt += payload_length;
    
//     if (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN))
//         connection->snd_nxt++;

//     TCP_SET_DATA_OFFSET(tcp_hdr, tcp_header_len / 4);
//     tcp_hdr->flags = flags;
//     tcp_hdr->window = bswap16(8192);
//     tcp_hdr->checksum = 0;
//     tcp_hdr->urgent_ptr = 0;
    
//     uint8_t* tcp_payload = tcp_packet + tcp_header_len;
//     if (p_payload && payload_length > 0)
//         memcpy(tcp_payload, p_payload, payload_length);

//     auto device = nidm_get_prefered_device(get_global_nidm());

//     tcp_hdr->checksum = tcp_checksum(device->ip.raw, connection->remote_ip, tcp_hdr, payload_length);

//     return ip_send(device, connection->remote_ip, IP_PROTOCOL_TCP, tcp_packet, total_tcp_len);
// }

// void tcp_receive(network_interface_device_t* device, uint8_t* payload, size_t payload_length, uint32_t src_ip) {
//     if (payload_length < sizeof(tcp_header_t)) {
//         kprintf("[INET - TCP: %s] packet too small\n", device->name.c_str());
//         return;
//     }

//     tcp_header_t* header = (tcp_header_t*)payload;

//     uint16_t src_port = bswap16(header->src_port);
//     uint16_t dst_port = bswap16(header->dst_port);
//     uint32_t seq_num = bswap32(header->seq_num);
//     uint32_t ack_num = bswap32(header->ack_num);
//     uint8_t data_offset = TCP_DATA_OFFSET(header);
//     uint8_t flags = header->flags;
//     uint16_t window = bswap16(header->window);

//     // kprintf("[INET - TCP: %s] got tcp packet for from: %u.%u.%u.%u:%u\n", device->name.c_str(), (src_ip >> 24) & 0xff, (src_ip >> 16) & 0xff, (src_ip >> 8) & 0xff, src_ip & 0xff, src_port);

//     size_t header_len = data_offset * 4;
//     if (header_len > payload_length) {
//         kprintf("[INET - TCP: %s] invalid header length\n", device->name.c_str());
//         return;
//     }

//     uint8_t* tcp_data = payload + header_len;
//     size_t tcp_data_len = payload_length - header_len;

//     tcp_connection_t* connection = nullptr;
//     for (auto& con : connections) {
//         if (con->local_ip == device->ip.raw &&
//             con->local_port == dst_port &&
//             con->remote_ip == src_ip &&
//             con->remote_port == src_port) {
//             connection = con.get();
//             break;
//         }

//         if (con->local_ip == device->ip.raw &&
//             con->local_port == dst_port &&
//             con->remote_ip == 0 &&
//             con->remote_port == 0) {
//             connection = con.get();
//             connection->remote_ip = src_ip;
//             connection->remote_port = src_port;
//             break;
//         }
//     }

//     // FIXME @since 23/09/2025 -- 18:21
//     // for now if we dont have a specific listener just close the connection
//     if (!connection) {
//         kprintf("[INET - TCP: %s] no handler\n", device->name.c_str());
//         tcp_send_packet(nullptr, 0, TCP_FLAG_RST, nullptr);
//         return;
//     }

//     if (flags & TCP_FLAG_RST) {
//         // connection aborted
//         connection->state = tcp_state_t::CLOSED;
//         return;
//     }

//     switch (connection->state) {
//         case tcp_state_t::LISTEN:
//             if (!(flags & TCP_FLAG_SYN))
//                 break;

//             connection->irs = seq_num;
//             connection->rcv_nxt = seq_num + 1;
//             connection->iss = (uint32_t)random_number(0, MAX_UINT32 - 1);
//             connection->snd_nxt = connection->iss;
//             connection->state = tcp_state_t::SYN_RECEIVED;
//             kprintf("[INET - TCP: %s] incoming connection attempt\n", device->name.c_str());
//             if (!tcp_send_packet(nullptr, 0, TCP_FLAG_SYN | TCP_FLAG_ACK, connection))
//                 connection->state = tcp_state_t::LISTEN;
//             break;
//         case tcp_state_t::SYN_SENT:
//             if (!(flags & TCP_FLAG_SYN) || !(flags & TCP_FLAG_ACK))
//                 break;
//             if (ack_num != connection->snd_nxt)
//                 break;

//             connection->state = tcp_state_t::ESTABLISHED;
//             connection->rcv_nxt = seq_num + 1;
//             connection->snd_una = ack_num;
//             kprintf("[INET - TCP: %s] accepted incoming connection\n", device->name.c_str());
//             tcp_send_packet(nullptr, 0, TCP_FLAG_ACK, connection);
//             break;
//         case tcp_state_t::SYN_RECEIVED:
//             if (!(flags & TCP_FLAG_ACK))
//                 break;

//             if (ack_num != connection->snd_nxt) {
//                 kprintf("[INET - TCP: %s] invalid ACK number, expected %u got %u\n", device->name.c_str(), connection->snd_nxt, ack_num);
//                 break;
//             }

//             kprintf("[INET - TCP: %s] connection established!\n", device->name.c_str());
//             connection->state = tcp_state_t::ESTABLISHED;
//             connection->snd_una = ack_num;
//             break;
//         case tcp_state_t::ESTABLISHED:
//             // TODO @since 23/09/2025 -- 19:58
//             // handle PSH
//             if (tcp_data_len > 0) {
//                 if (connection->callback)
//                     connection->callback(tcp_data, tcp_data_len);

//                 connection->rcv_nxt += tcp_data_len;
//                 tcp_send_packet(nullptr, 0, TCP_FLAG_ACK, connection);
//             }

//             if (flags & TCP_FLAG_FIN) {
//                 connection->rcv_nxt++;
//                 tcp_send_packet(nullptr, 0, TCP_FLAG_FIN | TCP_FLAG_ACK, connection);
//                 connection->state = tcp_state_t::CLOSE_WAIT;
//             }
//             break;
//         case tcp_state_t::CLOSE_WAIT:
//             break;
//         case tcp_state_t::LAST_ACK:
//             if ((flags & TCP_FLAG_ACK) && ack_num == connection->snd_nxt) {
//                 connection->state = tcp_state_t::CLOSED;
//                 // TODO @since 23/09/2025 -- 18:33
//                 // resource cleanup
//             }
//         break;
//         default:
//             kprintf("[INET - TCP: %s] unhandled connection state\n", device->name.c_str());
//             break;
//     }
// }