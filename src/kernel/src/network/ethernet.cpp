#include "network/ethernet.hpp"
#include "network/ip.hpp"
#include "network/arp.hpp"
#include "memory/heap.hpp"

void ethernet_send(network_interface_t* interface, uint8_t dst_mac[6], uint16_t type, const uint8_t* packet, size_t size) {
    const size_t frame_size = sizeof(ethernet_header_t) + size;

    network_packet_t network_packet {};
    network_packet.interface = interface;
    network_packet.data = (uint8_t*)malloc(frame_size);
    network_packet.size = frame_size;

    ethernet_header_t* frame_ethernet_header = (ethernet_header_t*)network_packet.data;
    memcpy(frame_ethernet_header->dst_mac, dst_mac, 6);
    memcpy(frame_ethernet_header->src_mac, interface->mac, 6);
    frame_ethernet_header->ethernet_type = bswap16(type);

    uint8_t* frame_packet = (uint8_t*)(network_packet.data + sizeof(ethernet_header_t));
    memcpy(frame_packet, packet, size);

    nic_dispatch_packet(get_global_nic(), network_packet);
}

int ethernet_receive(network_interface_t* interface, uint8_t* frame, size_t size) {
    if (size < sizeof(ethernet_header_t))
        return 1;

    const ethernet_header_t* header = (ethernet_header_t*)frame;
    uint8_t* payload = frame + sizeof(ethernet_header_t);
    const size_t payload_size = size - sizeof(ethernet_header_t);

    uint16_t ethertype = bswap16(header->ethernet_type);
    switch (ethertype) {
        case ETHERNET_TYPE_IPV4:
            ip_receive(interface, payload, payload_size);
            break;
        case ETHERNET_TYPE_ARP:
            arp_receive(interface, payload, payload_size);
            break;
        default:
            break;
    }

    return 0;
}