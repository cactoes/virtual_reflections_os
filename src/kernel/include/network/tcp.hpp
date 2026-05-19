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

// typedef void(*tcp_connect_callback_t)(const u8* data, size_t size);

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
    OPENED,
    SYN_SENT,
    FIN_WAIT,
    ESTABLISHED
};

struct tcp_header_t {
    u16 src_port;
    u16 dst_port;
    u32 seq_num;
    u32 ack_num;
    u8 data_offset_reserved;
    u8 flags;
    u16 window;
    u16 checksum;
    u16 urgent_ptr;
} PACKED;

struct tcb_t {
    u32 local_ip;
    u16 local_port;

    u32 remote_ip;
    u16 remote_port;

    u32 snd_nxt;
    u32 snd_una;
    u32 rcv_nxt;

    tcp_state_t state;
};

bool tcp_send(tcb_t* tcb, const u8* payload, size_t size);
tcb_t* tcp_create_tcb(u32 local_ip, u16 local_port, u32 remote_ip, u16 remote_port);
void tcp_receive(network_interface_t* interface, u32 src_ip, u8* payload, size_t payload_length);
bool tcp_is_connection_established(tcb_t* tcb);

// struct tcp_connection_t {


//     u32 snd_nxt;
//     u32 snd_una;
//     u32 rcv_nxt;
//     u32 iss;
//     u32 irs;

//     tcp_state_t state;
//     tcp_connect_callback_t callback;
// };

// tcp_connection_t* tcp_connect(u32 ip, u32 port, tcp_connect_callback_t callback);
// tcp_connection_t* tcp_listen(u32 port, tcp_connect_callback_t callback);
// bool tcp_send_packet(u8* p_payload, size_t payload_length, u8 flags, tcp_connection_t* connection);
// void tcp_receive(network_interface_device_t* p_device, u8* p_payload, size_t payload_length, u32 src_ip);

#endif // __DRIVERS_NETWORK_TCP_HPP__