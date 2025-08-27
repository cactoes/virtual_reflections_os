#include "drivers/network/ethernet.hpp"
#include "utils/map.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

// ARP SHIT BEGIN

// TODO @since 27/08/2025 -- 01:53
// timestamps / cache clearing idk
struct arp_table_entry_t {
    uint8_t mac[6];
};

static linear_map<uint32_t, arp_table_entry_t> g_address_lookup_table {};

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

void arp_table_insert(uint32_t ipv4, uint8_t p_mac[6]) {
    arp_table_entry_t entry {};
    memcpy(entry.mac, p_mac, 6);
    g_address_lookup_table[ipv4] = entry;
}

void arp_discover_request_ipv4(network_interface_device_t* p_device, uint32_t ipv4) {
    arp_packet_t arp_request {};
    arp_request.harware_type = htons(1);
    arp_request.protocol_type = htons(0x0800);
    arp_request.hardware_length = 6;
    arp_request.protocol_length = 4;
    arp_request.operation = htons(1);

    memcpy(arp_request.sender_hw, p_device->mac, 6);
    arp_request.sender_ip = htonl(p_device->ip4);

    static uint8_t s_broadcast_mac[] { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    memzero(arp_request.target_hw, 6);
    arp_request.target_ip = htonl(ipv4);

    ethernet_send(p_device, s_broadcast_mac, ETHERNET_TYPE_ARP, (uint8_t*)&arp_request, sizeof(arp_packet_t));
}

void arp_receive(network_interface_device_t* p_device, uint8_t* p_packet, size_t size) {
    arp_packet_t* arp_packet = (arp_packet_t*)p_packet;

    if (ntohs(arp_packet->harware_type) != 1 ||
        ntohs(arp_packet->protocol_type) != 0x0800 ||
        arp_packet->hardware_length != 6 ||
        arp_packet->protocol_length != 4) {
        return;
    }

    if (ntohs(arp_packet->operation) == 1 && ntohl(arp_packet->target_ip) == p_device->ip4) {
        printf(DBG, "[INET - ARP: %s] arp lookup for us\n", p_device->name.c_str());

        arp_packet_t arp_reply {};
        arp_reply.harware_type = htons(1);
        arp_reply.protocol_type = htons(0x0800);
        arp_reply.hardware_length = 6;
        arp_reply.protocol_length = 4;
        arp_reply.operation = htons(2);

        memcpy(arp_reply.sender_hw, p_device->mac, 6);
        arp_reply.sender_ip = htonl(p_device->ip4);

        memcpy(arp_reply.target_hw, arp_packet->sender_hw, 6);
        arp_reply.target_ip = arp_packet->sender_ip;

        ethernet_send(p_device, arp_packet->sender_hw, ETHERNET_TYPE_ARP, (uint8_t*)&arp_reply, sizeof(arp_packet_t));
    }

    if (ntohs(arp_packet->operation) == 2) {
        printf(DBG, "[INET - ARP: %s] new arp table entry\n", p_device->name.c_str());
        arp_table_insert(ntohl(arp_packet->sender_ip), arp_packet->sender_hw);
    }
}

bool arp_lookup(uint32_t ipv4_addr, uint8_t p_mac_out[6]) {
    auto it = g_address_lookup_table.get(ipv4_addr);
    if (it == g_address_lookup_table.end())
        return false;

    memcpy(p_mac_out, it->value.mac, 6);
    return true;
}

// ARP SHIT END

// IP SHIT BEGIN

#define IP_PROTOCOL_ICMP    1
#define IP_PROTOCOL_UDP     17

struct ip_header_t {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} PACKED;

struct icmp_header_t {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} PACKED;

uint16_t ip_checksum(void* p_data, size_t len) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)p_data;
    for (; len > 1; len -= 2) sum += *ptr++;
    if (len == 1) sum += *(uint8_t*)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

void ip_send(network_interface_device_t* p_device, uint32_t dst_ip, uint8_t protocol, const uint8_t* p_payload, size_t payload_len) {
    const auto total_length = sizeof(ip_header_t) + (payload_len > 1500 ? 1500 : payload_len);

    ip_header_t ip {};
    ip.version_ihl  = 0x45;
    ip.tos  = 0;
    ip.total_length = htons(total_length);
    ip.identification = 0;
    ip.flags_fragment = 0;
    ip.ttl  = 64;
    ip.protocol = protocol;
    ip.src_addr = htonl(p_device->ip4);
    ip.dst_addr = htonl(dst_ip);
    ip.header_checksum = 0;
    ip.header_checksum = ip_checksum(&ip, sizeof(ip));

    uint8_t dst_mac[6];
    if (!arp_lookup(dst_ip, dst_mac)) {
        printf(DBG, "[INET - IP: %s] arp lookup for %u.%u.%u.%u\n", p_device->name.c_str(), (dst_ip >> 24) & 0xff, (dst_ip >> 16) & 0xff, (dst_ip >> 8) & 0xff, (dst_ip) & 0xff);
        arp_discover_request_ipv4(p_device, dst_ip);
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

void icmp_receive(network_interface_device_t* p_device, uint8_t* p_payload, size_t len) {
    if (len < sizeof(icmp_header_t))
        return;

    len = len > 1500 ? 1500 : len;
    
    icmp_header_t* icmp = (icmp_header_t*)p_payload;

    if (icmp->type == 8) {
        printf(DBG, "[INET - ICMP: %s] get echo request\n", p_device->name.c_str());

        uint8_t reply_buf[1500];
        icmp_header_t* icmp_reply = (icmp_header_t*)reply_buf;
        memcpy(reply_buf, p_payload, len);
        icmp_reply->type = 0;
        icmp_reply->checksum = 0;
        icmp_reply->checksum = ip_checksum(reply_buf, len);

        ip_header_t* ip_req = (ip_header_t*)((uint8_t*)p_payload - sizeof(ip_header_t));
        ip_send(p_device, ntohl(ip_req->src_addr), IP_PROTOCOL_ICMP, reply_buf, len);
    }
}

void ip_receive(network_interface_device_t* p_device, uint8_t* p_packet, size_t size) {
    ip_header_t* ip = (ip_header_t*)p_packet;

    if (ntohl(ip->dst_addr) != p_device->ip4 && ntohl(ip->dst_addr) != 0xFFFFFFFF)
        return;
    
    uint16_t ip_len = ntohs(ip->total_length);
    uint8_t* payload = p_packet + (ip->version_ihl & 0x0F) * 4;
    size_t payload_len = ip_len - (ip->version_ihl & 0x0F) * 4;

    switch (ip->protocol) {
        case IP_PROTOCOL_ICMP:
            icmp_receive(p_device, payload, payload_len);
            break;
        // case IP_PROTOCOL_UDP:
        //     udp_receive(p_device, payload, payload_len);
        //     break;
    }
}

// IP SHIT END

void ethernet_send(network_interface_device_t* p_device, uint8_t p_dst_mac[6], uint16_t type, const uint8_t* p_packet, size_t size) {
    const size_t frame_size = sizeof(ethernet_header_t) + size;
    auto frame = (uint8_t*)heap_alloc(get_global_heap(), frame_size);

    auto frame_ethernet_header = (ethernet_header_t*)frame;
    memcpy(frame_ethernet_header->dst_mac, p_dst_mac, 6);
    memcpy(frame_ethernet_header->src_mac, p_device->mac, 6);
    frame_ethernet_header->ethernet_type = htons(type);

    auto frame_packet = (uint8_t*)(frame + sizeof(ethernet_header_t));
    memcpy(frame_packet, p_packet, size);

    UNUSED(nidm_send_data(p_device, frame, frame_size));

    heap_free(get_global_heap(), frame);
}

int ethernet_receive(network_interface_device_t* p_device, uint8_t* p_frame, size_t size) {
    if (size < sizeof(ethernet_header_t))
        return 1;

    const ethernet_header_t* header = (ethernet_header_t*)p_frame;
    uint8_t* payload = p_frame + sizeof(ethernet_header_t);
    const size_t payload_size = size - sizeof(ethernet_header_t);

    uint16_t ethertype = ntohs(header->ethernet_type);
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