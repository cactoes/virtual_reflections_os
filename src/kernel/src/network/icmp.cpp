#include "network/icmp.hpp"
#include "network/ip.hpp"
#include "io.hpp"

void icmp_receive(network_interface_device_t* p_device, uint8_t* p_payload, size_t len) {
    if (len < sizeof(icmp_header_t))
        return;

    len = len > 1500 ? 1500 : len;
    
    icmp_header_t* icmp = (icmp_header_t*)p_payload;

    if (icmp->type == 8) {
        kprintf("[INET - ICMP: %s] get echo request\n", p_device->name.c_str());

        uint8_t reply_buf[1500];
        icmp_header_t* icmp_reply = (icmp_header_t*)reply_buf;
        memcpy(reply_buf, p_payload, len);
        icmp_reply->type = 0;
        icmp_reply->checksum = 0;
        icmp_reply->checksum = ip_checksum(reply_buf, len);

        ip_header_t* ip_req = (ip_header_t*)((uint8_t*)p_payload - sizeof(ip_header_t));
        ip_send(p_device, bswap32(ip_req->src_addr), IP_PROTOCOL_ICMP, reply_buf, len);
    }
}