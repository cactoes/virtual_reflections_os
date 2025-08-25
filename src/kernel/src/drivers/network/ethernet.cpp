#include "drivers/network/ethernet.hpp"

// ARP SHIT BEGIN

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

#define ARP_TABLE_SIZE 16

struct arp_entry_t {
    uint32_t ip_addr;
    uint8_t mac_addr[6];
};

struct arp_table_t {
    arp_entry_t entries[ARP_TABLE_SIZE];
    int count;
};

struct arp_packet_t {
    uint16_t harware_type;
    uint16_t protocol_type;

    uint8_t hardware_length;
    uint8_t protocol_length;

    uint16_t operation;

    uint8_t sender_hw[6];
    uint32_t sender_ip;
    uint8_t target_hw[6];
    uint32_t target_ip;
} PACKED;

void arp_receive(network_interface_device_t* p_device, uint8_t* p_packet, size_t size) {
    arp_packet_t* arp_packet = (arp_packet_t*)p_packet;

    auto arp_op = ntohs(arp_packet->operation);

    printf(DBG, "someone is looking for: %uh\n", ntohl(arp_packet->target_ip));
    if (arp_op == 1 && ntohl(arp_packet->target_ip) == p_device->ip4) {
        printf(DBG, "someone is looking for us\n");
    }
}

// ARP SHIT END

int ethernet_receive(network_interface_device_t* p_device, uint8_t* p_frame, size_t size) {
    if (size < sizeof(ethernet_header_t))
        return 1;

    const ethernet_header_t* header = (ethernet_header_t*)p_frame;
    uint8_t* payload = p_frame + sizeof(ethernet_header_t);
    const size_t payload_size = size - sizeof(ethernet_header_t);

    uint16_t ethertype = ntohs(header->ethernet_type);
    switch (ethertype) {
        case ETHERNET_TYPE_IPV4:
            // ip_receive(p_device, payload, payload_size);
            break;
        case ETHERNET_TYPE_ARP:
            arp_receive(p_device, payload, payload_size);
            break;
        default:
            // Unknown protocol
            break;
    }

    return 0;
}