//==========================================
/// @file       tcp.hpp
/// @brief      transmission control protocol
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_TCP_HPP__
#define __DRIVERS_NETWORK_TCP_HPP__

#define TCP_FLAG_FIN    (1 << 0)
#define TCP_FLAG_SYN    (1 << 1)
#define TCP_FLAG_RST    (1 << 2)
#define TCP_FLAG_PSH    (1 << 3)
#define TCP_FLAG_ACK    (1 << 4)
#define TCP_FLAG_URG    (1 << 5)
#define TCP_FLAG_ECE    (1 << 6)
#define TCP_FLAG_CWR    (1 << 7)

#define TCP_DATA_OFFSET(hdr) ((hdr)->data_offset_reserved >> 4)
#define TCP_SET_DATA_OFFSET(hdr, offset) ((hdr)->data_offset_reserved = ((offset) << 4) | ((hdr)->data_offset_reserved & 0x0F))

#include "common.hpp"
#include "network/nidm.hpp"

// typedef void(*tcp_connect_callback_t)(const uint8_t* data, size_t size);

enum class tcp_state_t {
    // CLOSED,
    // LISTEN,
    // SYN_SENT,
    // SYN_RECEIVED,
    // ESTABLISHED,
    // FIN_WAIT1,
    // FIN_WAIT2,
    // CLOSE_WAIT,
    // CLOSING,
    // LAST_ACK,
    // TIME_WAIT

    CLOSED,
    SYN_SENT,
    FIN_WAIT,
    ESTABLISHED
};

struct tcp_header_t {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset_reserved;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} PACKED;

struct tcb_t {
    uint32_t local_ip;
    uint16_t local_port;

    uint32_t remote_ip;
    uint16_t remote_port;

    uint32_t snd_nxt;
    uint32_t rcv_nxt;

    tcp_state_t state;
};

bool tcp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const uint8_t* payload, size_t size);

// struct tcp_connection_t {


//     uint32_t snd_nxt;
//     uint32_t snd_una;
//     uint32_t rcv_nxt;
//     uint32_t iss;
//     uint32_t irs;

//     tcp_state_t state;
//     tcp_connect_callback_t callback;
// };

// tcp_connection_t* tcp_connect(uint32_t ip, uint32_t port, tcp_connect_callback_t callback);
// tcp_connection_t* tcp_listen(uint32_t port, tcp_connect_callback_t callback);
// bool tcp_send_packet(uint8_t* p_payload, size_t payload_length, uint8_t flags, tcp_connection_t* connection);
// void tcp_receive(network_interface_device_t* p_device, uint8_t* p_payload, size_t payload_length, uint32_t src_ip);

#endif // __DRIVERS_NETWORK_TCP_HPP__