#include "drivers/network/udp.hpp"
#include "drivers/network/ip.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void udp_receive(network_interface_device_t* p_device, uint8_t* p_payload, size_t payload_length) {
    udp_header_t* header = (udp_header_t*)p_payload;
    uint16_t src_port = net_to_host(header->src_port);
    uint16_t dst_port = net_to_host(header->dst_port);
    uint16_t length = net_to_host(header->length);
    uint16_t checksum = net_to_host(header->checksum);

    if (length != payload_length)
        printf(DBG, "[INET - UDP: %s] payload length mismatch!\n", p_device->name.c_str());

    uint8_t* udp_payload = p_payload + sizeof(udp_header_t);
    size_t udp_payload_length = payload_length - sizeof(udp_header_t);

    nidm_udp_dispatch(get_global_nidm(), dst_port, udp_payload, udp_payload_length);
}

int udp_send(network_interface_device_t* p_device, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const uint8_t* p_payload, size_t size) {
    if (size > 1500 - sizeof(ip_header_t) - sizeof(udp_header_t))
        return 1;

    const size_t packet_size = sizeof(udp_header_t) + size;
    uint8_t* packet = (uint8_t*)heap_alloc(get_global_heap(), packet_size);

    if (!packet)
        return 2;

    udp_header_t* header = (udp_header_t*)packet;
    header->src_port = host_to_net<uint16_t>(src_port);
    header->dst_port = host_to_net<uint16_t>(dst_port);
    // TODO @since 29/08/2025 -- 19:39
    header->checksum = 0;
    header->length = host_to_net<uint16_t>(packet_size);

    memcpy(packet + sizeof(udp_header_t), p_payload, size);
    
    ip_send(p_device, dst_ip, IP_PROTOCOL_UDP, packet, packet_size);

    heap_free(get_global_heap(), packet);

    return 0;
}