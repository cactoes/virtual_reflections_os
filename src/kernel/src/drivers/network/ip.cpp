#include "drivers/network/ip.hpp"
#include "drivers/network/ethernet.hpp"
#include "drivers/network/icmp.hpp"
#include "drivers/network/udp.hpp"
#include "drivers/network/arp.hpp"
#include "drivers/network/tcp.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void ip_receive(network_interface_device_t* p_device, uint8_t* p_packet, size_t size) {
    ip_header_t* ip = (ip_header_t*)p_packet;

    if (net_to_host(ip->dst_addr) != p_device->ip4 && net_to_host(ip->dst_addr) != 0xFFFFFFFF)
        return;
    
    uint16_t ip_len = net_to_host(ip->total_length);
    uint8_t* payload = p_packet + (ip->version_ihl & 0x0F) * 4;
    // BUG @since 28/08/2025 -- 18:30
    // size can differ from actual length
    size_t payload_len = ip_len - (ip->version_ihl & 0x0F) * 4;

    switch (ip->protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_receive(p_device, payload, payload_len);
            break;
        case IP_PROTOCOL_TCP:
            tcp_receive(p_device, payload, payload_len, net_to_host(ip->src_addr));
            break;
        case IP_PROTOCOL_UDP:
            udp_receive(p_device, payload, payload_len);
            break;
    }
}

bool is_local_network(uint32_t target_ip, uint32_t our_ip, uint32_t subnet_mask) {
    return (target_ip & subnet_mask) == (our_ip & subnet_mask);
}

uint32_t get_next_hop_ip(network_interface_device_t* device, uint32_t target_ip) {
    return is_local_network(target_ip, device->ip4, device->subnet_mask) ? target_ip : device->gateway_ip;
}

void ip_send(network_interface_device_t* p_device, uint32_t dst_ip, uint8_t protocol, const uint8_t* p_payload, size_t payload_len) {
    const auto total_length = sizeof(ip_header_t) + (payload_len > 1500 ? 1500 : payload_len);

    ip_header_t ip {};
    ip.version_ihl = 0x45;
    ip.tos = 0;
    ip.total_length = host_to_net<uint16_t>(total_length);
    ip.identification = 0;
    ip.flags_fragment = 0;
    ip.ttl = 64;
    ip.protocol = protocol;
    ip.src_addr = htonl(p_device->ip4);
    ip.dst_addr = htonl(dst_ip);
    ip.header_checksum = 0;
    ip.header_checksum = ip_checksum(&ip, sizeof(ip));

    uint32_t next_net_hop_ip = get_next_hop_ip(p_device, dst_ip);

    uint8_t dst_mac[6];
    if (!arp_lookup(next_net_hop_ip, dst_mac)) {
        printf(DBG, "[INET - IP: %s] arp lookup for %u.%u.%u.%u\n", p_device->name.c_str(), (next_net_hop_ip >> 24) & 0xff, (next_net_hop_ip >> 16) & 0xff, (next_net_hop_ip >> 8) & 0xff, (next_net_hop_ip) & 0xff);
        arp_discover_request_ipv4(p_device, next_net_hop_ip);
        return;
    }

    struct {
        ip_header_t ip;
        uint8_t payload[1500];
    } PACKED ip_payload;

    ip_payload.ip = ip;
    memcpy(ip_payload.payload, p_payload, payload_len > 1500 ? 1500 : payload_len);

    ethernet_send(p_device, dst_mac, ETHERNET_TYPE_IPV4, (uint8_t*)&ip_payload, total_length);
}

uint16_t ip_checksum(void* p_data, size_t len) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)p_data;
    for (; len > 1; len -= 2) sum += *ptr++;
    if (len == 1) sum += *(uint8_t*)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}