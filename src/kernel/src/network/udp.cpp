#include "network/udp.hpp"
#include "network/ip.hpp"
#include "memory/heap.hpp"
#include "io.hpp"
#include "network/socket.hpp"

void udp_receive(network_interface_t* interface, u32 src_ip, u8* payload, size_t payload_length) {
    udp_header_t* header = (udp_header_t*)payload;
    u16 src_port = bswap16(header->src_port);
    u16 dst_port = bswap16(header->dst_port);
    u16 length = bswap16(header->length);
    u16 checksum = bswap16(header->checksum);

    if (length != payload_length)
        kprintf("[INET - UDP: %s] payload length mismatch!\n", interface->device_name);

    u8* udp_payload = payload + sizeof(udp_header_t);
    size_t udp_payload_length = payload_length - sizeof(udp_header_t);

    socket_receive(socket_protocol_t::UDP, dst_port, src_ip, src_port, udp_payload, udp_payload_length);
}

u16 udp_checksum(u32 src_ip, u32 dst_ip, const u8* udp_packet, size_t udp_len) {
    u32 sum = 0;
    
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += 17;
    sum += udp_len;
    
    const u8* ptr = udp_packet;
    size_t len = udp_len;
    
    while (len > 1) {
        sum += ((u16)ptr[0] << 8) | ptr[1];
        ptr += 2;
        len -= 2;
    }
    
    if (len > 0)
        sum += (u16)ptr[0] << 8;
    
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    
    u16 result = (u16)~sum;
    
    if (result == 0x0000)
        result = 0xFFFF;
    
    return result;
}

bool udp_send(u32 dst_ip, u16 src_port, u16 dst_port, const u8* p_payload, size_t size) {
    if (size > 1500 - sizeof(ip_header_t) - sizeof(udp_header_t))
        return false;

    const size_t packet_size = sizeof(udp_header_t) + size;
    u8* packet = (u8*)malloc(packet_size);

    if (!packet)
        return false;

    udp_header_t* header = (udp_header_t*)packet;
    header->src_port = bswap16(src_port);
    header->dst_port = bswap16(dst_port);
    header->length = bswap16(packet_size);
    header->checksum = 0;

    memcpy(packet + sizeof(udp_header_t), p_payload, size);

    network_interface_t* interface = route_lookup(get_global_nic(), dst_ip);

    if (!interface) {
        free(packet);
        return false;
    }

    header->checksum = bswap16(udp_checksum(
        interface->ip.raw,
        dst_ip,
        packet,
        packet_size
    ));

    ip_send(interface, dst_ip, IP_PROTOCOL_UDP, packet, packet_size);

    free(packet);

    return 1;
}