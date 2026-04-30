#include "network/icmp.hpp"
#include "network/ip.hpp"
#include "io.hpp"

uint16_t icmp_checksum(void* p_data, size_t len) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)p_data;
    for (; len > 1; len -= 2) sum += *ptr++;
    if (len == 1) sum += *(uint8_t*)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

void icmp_receive(network_interface_t* interface, uint8_t* payload, size_t len) {
    if (len < sizeof(icmp_header_t))
        return;

    len = len > 1500 ? 1500 : len;
    
    icmp_header_t* icmp = (icmp_header_t*)payload;

    if (icmp->type == 8) {
        kprintf("[INET - ICMP: %s] get echo request\n", interface->device_name);

        uint8_t reply_buf[1500];
        icmp_header_t* icmp_reply = (icmp_header_t*)reply_buf;
        memcpy(reply_buf, payload, len);
        icmp_reply->type = 0;
        icmp_reply->checksum = 0;
        icmp_reply->checksum = icmp_checksum(reply_buf, len);

        ip_header_t* ip_req = (ip_header_t*)((uint8_t*)payload - sizeof(ip_header_t));
        ip_send(interface, bswap32(ip_req->src_addr), IP_PROTOCOL_ICMP, reply_buf, len);
    }
}