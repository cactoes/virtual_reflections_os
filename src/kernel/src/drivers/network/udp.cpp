#include "drivers/network/udp.hpp"

enum print_mode_t {
    STD,
    DBG
};

extern void printf(print_mode_t mode, const char* p_str, ...);

void udp_receive(network_interface_device_t* p_device, uint8_t* p_payload, size_t payload_length) {
    udp_header_t* header = (udp_header_t*)p_payload;
    uint16_t src_port = ntohs(header->src_port);
    uint16_t dst_port = ntohs(header->dst_port);
    uint16_t length = ntohs(header->length);
    uint16_t checksum = ntohs(header->checksum);

    if (length != payload_length)
        printf(DBG, "[INET - UDP: %s] payload length mismatch!\n", p_device->name.c_str());

    uint8_t* udp_payload = p_payload + sizeof(udp_header_t);
    size_t udp_payload_length = payload_length - sizeof(udp_header_t);

    printf(DBG, "[INET - UDP: %s]: got packet for port (%u bytes): %u\n", p_device->name.c_str(), udp_payload_length, dst_port);
}