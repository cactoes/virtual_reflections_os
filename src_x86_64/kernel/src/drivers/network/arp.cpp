#include "drivers/network/arp.hpp"
#include "drivers/network/ethernet.hpp"
#include "time/clock.hpp"
#include "std/map.hpp"
#include "io.hpp"

static std::linear_map<uint32_t, arp_table_entry_t> g_address_lookup_table {};

void arp_table_insert(uint32_t ipv4, uint8_t p_mac[6]) {
    arp_table_entry_t entry {};
    entry.timestamp = clock_get_time_since_boot();
    memcpy(entry.mac, p_mac, 6);
    g_address_lookup_table[ipv4] = entry;
}

void arp_discover_request_ipv4(network_interface_device_t* p_device, uint32_t ipv4) {
    arp_packet_t arp_request {};
    arp_request.harware_type = bswap16(1);
    arp_request.protocol_type = bswap16(0x0800);
    arp_request.hardware_length = 6;
    arp_request.protocol_length = 4;
    arp_request.operation = bswap16(1);

    memcpy(arp_request.sender_hw, p_device->mac, 6);
    arp_request.sender_ip = bswap32(p_device->ip.raw);

    static uint8_t s_broadcast_mac[] { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    memzero(arp_request.target_hw, 6);
    arp_request.target_ip = bswap32(ipv4);

    ethernet_send(p_device, s_broadcast_mac, ETHERNET_TYPE_ARP, (uint8_t*)&arp_request, sizeof(arp_packet_t));
}

void arp_receive(network_interface_device_t* p_device, uint8_t* p_packet, size_t size) {
    arp_packet_t* arp_packet = (arp_packet_t*)p_packet;

    if (bswap16(arp_packet->harware_type) != 1 ||
        bswap16(arp_packet->protocol_type) != 0x0800 ||
        arp_packet->hardware_length != 6 ||
        arp_packet->protocol_length != 4) {
        return;
    }

    if (bswap16(arp_packet->operation) == 1 && bswap32(arp_packet->target_ip) == p_device->ip.raw) {
        kprintf("[INET - ARP: %s] arp lookup for us\n", p_device->name.c_str());

        arp_packet_t arp_reply {};
        arp_reply.harware_type = bswap16(1);
        arp_reply.protocol_type = bswap16(0x0800);
        arp_reply.hardware_length = 6;
        arp_reply.protocol_length = 4;
        arp_reply.operation = bswap16(2);

        memcpy(arp_reply.sender_hw, p_device->mac, 6);
        arp_reply.sender_ip = bswap32(p_device->ip.raw);

        memcpy(arp_reply.target_hw, arp_packet->sender_hw, 6);
        arp_reply.target_ip = arp_packet->sender_ip;

        ethernet_send(p_device, arp_packet->sender_hw, ETHERNET_TYPE_ARP, (uint8_t*)&arp_reply, sizeof(arp_packet_t));
    }

    if (bswap16(arp_packet->operation) == 2) {
        kprintf("[INET - ARP: %s] new arp table entry\n", p_device->name.c_str());
        arp_table_insert(bswap32(arp_packet->sender_ip), arp_packet->sender_hw);
    }
}

bool arp_lookup(uint32_t ipv4_addr, uint8_t p_mac_out[6]) {
    auto it = g_address_lookup_table.get(ipv4_addr);
    if (it == g_address_lookup_table.end())
        return false;

    memcpy(p_mac_out, it->value.mac, 6);
    return true;
}