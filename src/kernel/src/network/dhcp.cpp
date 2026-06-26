#include "network/dhcp.hpp"
#include "std/random.hpp"
#include "time/clock.hpp"

bool dhcp_options_writer_init(dhcp_options_writer_t* writer, u8* buffer, size_t size) {
    if (!writer || !buffer)
        return false;

    writer->buffer = buffer;
    writer->buffer_size = size;
    writer->offset = 0;

    return true;
}

bool dhcp_options_writer_add_option(dhcp_options_writer_t* writer, u8 type, u8* data, size_t size) {
    if (writer->offset + size + 2 > writer->buffer_size)
        return false;

    writer->buffer[writer->offset++] = type;
    writer->buffer[writer->offset++] = size;
    memcpy(&writer->buffer[writer->offset], data, size);
    writer->offset += size;

    return true;
}

bool dhcp_options_writer_shutdown(dhcp_options_writer_t* writer) {
    if (writer->offset >= writer->buffer_size)
        return false;

    writer->buffer[writer->offset++] = DHCP_OPTION_END;
    return true;
}

u8* dhcp_option_get(dhcp_packet_t* packet, u8 type) {
    u8* options = &packet->options[0];
    size_t max_size = sizeof(packet->options);

    size_t ptr = 0;
    while (ptr < max_size) {
        if (options[0] == type)
            return options;

        if (options[0] == DHCP_OPTION_END)
            return nullptr;

        if (options[0] == DHCP_OPTION_PAD) {
            options++;
            ptr++;
            continue;
        }

        options += options[1] + 2;
        ptr += options[1] + 2;
    }

    return nullptr;
}

u32 dhcp_generate_xid() {
    seed_random(clock_get_time_since_boot());
    return (u32)random_number(0, MAX_UINT32);
}

u32 dhcp_field_to_number(u8* field, size_t size) {
    u32 number = 0;
    for (size_t i = 0; i < size; i++) {
        number <<= 8;
        number += field[i];
    }

    return number;
}

dhcp_packet_t dhcp_create_discover_packet(const char* hostname, dhcp_client_t::session_t* session) {
    dhcp_packet_t packet {};
    memzero(&packet, sizeof(dhcp_packet_t));

    if (!session)
        return packet;

    packet.op = DHCP_OP_BOOTREQUEST;
    packet.htype = DHCP_HTYPE_ETHERNET;
    packet.hlen = DHCP_HLEN_ETHERNET_ADDRESS;
    packet.hops = 0;

    packet.xid = session->xid;
    packet.secs = bswap16(0);
    packet.flags = bswap16(0x8000);

    packet.client_ip_addr = bswap32(0);
    packet.your_ip_addr = bswap32(0);
    packet.server_ip_addr = bswap32(0);
    packet.gateway_ip_addr = bswap32(0);

    for (size_t i = 0; i < 6; i++)
        packet.client_hw_addr[i] = session->interface->mac[i];

    packet.magic = bswap32(DHCP_MAGIC);

    dhcp_options_writer_t writer {};
    dhcp_options_writer_init(&writer, &packet.options[0], sizeof(dhcp_packet_t::options));

    u8 dhcp_message_type[] { DHCP_MESSAGE_TYPE_DHCPDISCOVER };
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_DHCP_MESSAGE_TYPE, &dhcp_message_type[0], sizeof(dhcp_message_type));

    u8 client_id[] { DHCP_HTYPE_ETHERNET, session->interface->mac[0], session->interface->mac[1], session->interface->mac[2],session->interface-> mac[3], session->interface->mac[4], session->interface->mac[5] };
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_CLIENT_ID, &client_id[0], sizeof(client_id));

    dhcp_options_writer_add_option(&writer, DHCP_OPTION_HOSTNAME, (u8*)hostname, strlen(hostname));

    u8 param_request_list[] { DHCP_OPTION_SUBNET_MASK, DHCP_OPTION_ROUTER, DHCP_OPTION_DNS };
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_PARAMETER_REQUEST_LIST, &param_request_list[0], sizeof(param_request_list));

    dhcp_options_writer_shutdown(&writer);

    return packet;
}

dhcp_packet_t dhcp_create_request_packet(const char* hostname, dhcp_client_t::session_t* session, u32 wanted_ip) {
    dhcp_packet_t packet {};
    memzero(&packet, sizeof(dhcp_packet_t));

    if (!session)
        return packet;

    packet.op = DHCP_OP_BOOTREQUEST;
    packet.htype = DHCP_HTYPE_ETHERNET;
    packet.hlen = DHCP_HLEN_ETHERNET_ADDRESS;
    packet.hops = 0;

    packet.xid = session->xid;
    packet.secs = bswap16(0);
    packet.flags = bswap16(0x8000);

    packet.client_ip_addr = bswap32(0);
    packet.your_ip_addr = bswap32(0);
    packet.server_ip_addr = bswap32(0);
    packet.gateway_ip_addr = bswap32(0);

    for (size_t i = 0; i < 6; i++)
        packet.client_hw_addr[i] = session->interface->mac[i];

    packet.magic = bswap32(DHCP_MAGIC);

    dhcp_options_writer_t writer {};
    dhcp_options_writer_init(&writer, &packet.options[0], sizeof(dhcp_packet_t::options));

    u8 dhcp_message_type[] { DHCP_MESSAGE_TYPE_DHCPREQUEST };
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_DHCP_MESSAGE_TYPE, &dhcp_message_type[0], sizeof(dhcp_message_type));

    u8 client_id[] { DHCP_HTYPE_ETHERNET, session->interface->mac[0], session->interface->mac[1], session->interface->mac[2],session->interface-> mac[3], session->interface->mac[4], session->interface->mac[5] };
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_CLIENT_ID, &client_id[0], sizeof(client_id));

    u32 wanted_ip_swapped = bswap32(wanted_ip);
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_REQUESTED_IP_ADDR, (u8*)&wanted_ip_swapped, sizeof(u32));

    u32 server_id_swapped = bswap32(session->dhcp_ip.raw);
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_DHCP_SERVER_ID, (u8*)&server_id_swapped, sizeof(u32));

    dhcp_options_writer_add_option(&writer, DHCP_OPTION_HOSTNAME, (u8*)hostname, strlen(hostname));

    u8 param_request_list[] { DHCP_OPTION_SUBNET_MASK, DHCP_OPTION_ROUTER, DHCP_OPTION_DNS };
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_PARAMETER_REQUEST_LIST, &param_request_list[0], sizeof(param_request_list));

    dhcp_options_writer_shutdown(&writer);

    return packet;
}

dhcp_packet_t dhcp_create_lease_extend_packet(const char* hostname, dhcp_client_t::session_t* session, u32 ip_to_extend) {
    dhcp_packet_t packet {};
    memzero(&packet, sizeof(dhcp_packet_t));

    if (!session)
        return packet;

    packet.op = DHCP_OP_BOOTREQUEST;
    packet.htype = DHCP_HTYPE_ETHERNET;
    packet.hlen = DHCP_HLEN_ETHERNET_ADDRESS;
    packet.hops = 0;

    packet.xid = session->xid;
    packet.secs = bswap16(0);
    packet.flags = bswap16(0);

    packet.client_ip_addr = bswap32(ip_to_extend);
    packet.your_ip_addr = bswap32(0);
    packet.server_ip_addr = bswap32(session->dhcp_ip.raw);
    packet.gateway_ip_addr = bswap32(0);

    for (size_t i = 0; i < 6; i++)
        packet.client_hw_addr[i] = session->interface->mac[i];

    packet.magic = bswap32(DHCP_MAGIC);

    dhcp_options_writer_t writer {};
    dhcp_options_writer_init(&writer, &packet.options[0], sizeof(dhcp_packet_t::options));

    u8 dhcp_message_type[] { DHCP_MESSAGE_TYPE_DHCPREQUEST };
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_DHCP_MESSAGE_TYPE, &dhcp_message_type[0], sizeof(dhcp_message_type));

    u32 ip_to_extend_swapped = bswap32(ip_to_extend);
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_REQUESTED_IP_ADDR, (u8*)&ip_to_extend_swapped, sizeof(u32));

    u32 dhcp_server_id_swapped = bswap32(session->dhcp_ip.raw);
    dhcp_options_writer_add_option(&writer, DHCP_OPTION_DHCP_SERVER_ID, (u8*)&dhcp_server_id_swapped, sizeof(u32));

    dhcp_options_writer_add_option(&writer, DHCP_OPTION_HOSTNAME, (u8*)hostname, strlen(hostname));

    dhcp_options_writer_shutdown(&writer);

    return packet;
}

dhcp_client_t* dhcp_client_create() {
    dhcp_client_t* client = new dhcp_client_t {};
    mutex_init(&client->mutex);
    return client;
}

dhcp_client_t::session_t dhcp_client_create_session(network_interface_t* target_interface) {
    return dhcp_client_t::session_t {
        .xid = dhcp_generate_xid(),
        .ip = {},
        .dhcp_ip = {},
        .lease_time = 0,
        .interface = target_interface
    };
}

bool dhcp_client_destroy(dhcp_client_t* client) {
    if (!client)
        return false;

    delete client;

    return true;
}