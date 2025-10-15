#include "drivers/network/ethernet.hpp"
#include "drivers/network/ip.hpp"
#include "drivers/network/arp.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void ethernet_send(network_interface_device_t* p_device, uint8_t p_dst_mac[6], uint16_t type, const uint8_t* p_packet, size_t size) {
    const size_t frame_size = sizeof(ethernet_header_t) + size;
    auto frame = (uint8_t*)heap_alloc(get_global_heap(), frame_size);

    auto frame_ethernet_header = (ethernet_header_t*)frame;
    memcpy(frame_ethernet_header->dst_mac, p_dst_mac, 6);
    memcpy(frame_ethernet_header->src_mac, p_device->mac, 6);
    frame_ethernet_header->ethernet_type = host_to_net<uint16_t>(type);

    auto frame_packet = (uint8_t*)(frame + sizeof(ethernet_header_t));
    memcpy(frame_packet, p_packet, size);

    UNUSED(nidm_packet_send(get_global_nidm(), frame, frame_size));

    heap_free(get_global_heap(), frame);
}

int ethernet_receive(network_interface_device_t* p_device, uint8_t* p_frame, size_t size) {
    if (size < sizeof(ethernet_header_t))
        return 1;

    const ethernet_header_t* header = (ethernet_header_t*)p_frame;
    uint8_t* payload = p_frame + sizeof(ethernet_header_t);
    const size_t payload_size = size - sizeof(ethernet_header_t);

    uint16_t ethertype = net_to_host(header->ethernet_type);
    switch (ethertype) {
        case ETHERNET_TYPE_IPV4:
            ip_receive(p_device, payload, payload_size);
            break;
        case ETHERNET_TYPE_ARP:
            arp_receive(p_device, payload, payload_size);
            break;
        default:
            break;
    }

    return 0;
}