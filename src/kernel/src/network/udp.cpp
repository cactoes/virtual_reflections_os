#include "network/udp.hpp"
#include "network/ip.hpp"
#include "memory/heap.hpp"
#include "io.hpp"
#include "network/socket.hpp"

void udp_receive(network_interface_t* interface, uint32_t src_ip, uint8_t* payload, size_t payload_length) {
    udp_header_t* header = (udp_header_t*)payload;
    uint16_t src_port = bswap16(header->src_port);
    uint16_t dst_port = bswap16(header->dst_port);
    uint16_t length = bswap16(header->length);
    uint16_t checksum = bswap16(header->checksum);

    if (length != payload_length)
        kprintf("[INET - UDP: %s] payload length mismatch!\n", interface->device_name);

    uint8_t* udp_payload = payload + sizeof(udp_header_t);
    size_t udp_payload_length = payload_length - sizeof(udp_header_t);

    socket_receive(socket_protocol_t::UDP, dst_port, src_ip, src_port, udp_payload, udp_payload_length);
}

uint16_t udp_checksum(uint32_t src_ip, uint32_t dst_ip, const uint8_t* udp_packet, size_t udp_len) {
    uint32_t sum = 0;
    
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += 17;
    sum += udp_len;
    
    const uint8_t* ptr = udp_packet;
    size_t len = udp_len;
    
    while (len > 1) {
        sum += ((uint16_t)ptr[0] << 8) | ptr[1];
        ptr += 2;
        len -= 2;
    }
    
    if (len > 0)
        sum += (uint16_t)ptr[0] << 8;
    
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    
    uint16_t result = (uint16_t)~sum;
    
    if (result == 0x0000)
        result = 0xFFFF;
    
    return result;
}

bool udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const uint8_t* p_payload, size_t size) {
    if (size > 1500 - sizeof(ip_header_t) - sizeof(udp_header_t))
        return false;

    const size_t packet_size = sizeof(udp_header_t) + size;
    uint8_t* packet = (uint8_t*)malloc(packet_size);

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