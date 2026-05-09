#include "network/network_manager.hpp"
#include "io.hpp"
#include "time/clock.hpp"
#include "virtual_thread.hpp"
#include "std/random.hpp"
#include "network/ip.hpp"

static network_manager_t* global_network_manager = nullptr;

network_manager_t* get_global_network_manager() {
    return global_network_manager;
}

void set_global_network_manager(network_manager_t* network_manager) {
    global_network_manager = network_manager;
}

bool network_manager_init(network_manager_t* network_manager) {
    if (!network_manager)
        return false;

    network_manager->dhcp_client = nullptr;
    network_manager->dhcp_socket = nullptr;
    network_manager->dns_client = nullptr;
    network_manager->dns_socket = nullptr;

    return true;
}

void network_manager_dhcp_callback(socket_t* socket, uint32_t ip, uint16_t port, const uint8_t* data, size_t size) {
    if (size > sizeof(dhcp_packet_t))
        return;

    network_manager_t* network_manager = get_global_network_manager();
    if (!network_manager)
        return;

    dhcp_packet_t* packet = (dhcp_packet_t*)data;

    dhcp_client_t::session_t& session = network_manager->dhcp_client->session;
    if (packet->xid != session.xid)
        return;

    dhcp_option_t<1>* option_message_type = (dhcp_option_t<1>*)dhcp_option_get(packet, DHCP_OPTION_DHCP_MESSAGE_TYPE);
    if (!option_message_type)
        return;

    dhcp_option_t<4>* option_dhcp_server_id = (dhcp_option_t<4>*)dhcp_option_get(packet, DHCP_OPTION_DHCP_SERVER_ID);
    dhcp_option_t<4>* option_router = (dhcp_option_t<4>*)dhcp_option_get(packet, DHCP_OPTION_ROUTER);
    dhcp_option_t<4>* option_lease_time_s = (dhcp_option_t<4>*)dhcp_option_get(packet, DHCP_OPTION_IP_LEASE_TIME);
    dhcp_option_t<4>* option_subnet_mask = (dhcp_option_t<4>*)dhcp_option_get(packet, DHCP_OPTION_SUBNET_MASK);

    if (option_message_type->value[0] == DHCP_MESSAGE_TYPE_DHCPOFFER) {
        if (!option_dhcp_server_id || !option_lease_time_s)
            return;

        session.ip = { .raw = bswap32(packet->your_ip_addr) };
        session.dhcp_ip = { .raw = TO_IP(option_dhcp_server_id->value[0], option_dhcp_server_id->value[1], option_dhcp_server_id->value[2], option_dhcp_server_id->value[3]) };
        session.lease_time = dhcp_field_to_number(&option_lease_time_s->value[0], 4);

        dhcp_packet_t p = dhcp_create_request_packet(DEVICE_HOST_NAME, &session, session.ip.raw);
        network_manager->dhcp_socket->remote_ip = session.dhcp_ip.raw;
        socket_send(network_manager->dhcp_socket, (const uint8_t*)&p, sizeof(dhcp_packet_t));
    }

    if (option_message_type->value[0] == DHCP_MESSAGE_TYPE_DHCPACK) {
        if (!option_subnet_mask || !option_router)
            return;

        if (bswap32(packet->your_ip_addr) != session.ip.raw)
            return;

        session.interface->subnet_mask = { .raw = TO_IP(option_subnet_mask->value[0], option_subnet_mask->value[1], option_subnet_mask->value[2], option_subnet_mask->value[3]) };
        session.interface->gateway = { .raw = TO_IP(option_router->value[0], option_router->value[1], option_router->value[2], option_router->value[3]) };
        session.interface->ip = session.ip;
        session.interface->is_active = true;

        kprintf("[DHCP] configured ip for '%s' - %u.%u.%u.%u\n", session.interface->device_name, (uint32_t)session.ip.byte3, (uint32_t)session.ip.byte2, (uint32_t)session.ip.byte1, (uint32_t)session.ip.byte0);
    }
}

bool network_manager_dhcp_send_discover(network_manager_t* network_manager) {
    if (!network_manager)
        return false;

    dhcp_packet_t packet = dhcp_create_discover_packet(DEVICE_HOST_NAME, &network_manager->dhcp_client->session);
    return socket_send(network_manager->dhcp_socket, (uint8_t*)&packet, sizeof(dhcp_packet_t));
}

bool network_manager_configre_interface(network_manager_t* network_manager, network_interface_t* interface) {
    if (!network_manager || !interface)
        return false;

    // only support one session so we need to check if we already have a device asigned
    if (network_manager->dhcp_client->session.interface)
        return false;

    network_manager->dhcp_client->session = dhcp_client_create_session(interface);

    return network_manager_dhcp_send_discover(network_manager);
}

bool network_manager_dns_send_query(network_manager_t* network_manager, const char* hostname) {
    if (!network_manager)
        return false;

    size_t query_size = 0;
    uint8_t* query = dns_create_query_packet(hostname, dns_query_type_t::A, &query_size);
    if (!query)
        return false;

    return socket_send(network_manager->dns_socket, query, query_size);
}

uint32_t network_manager_dns_query(network_manager_t* network_manager, const char* hostname) {
    if (!network_manager || !hostname)
        return 0;

    if (auto record = dns_client_get_record(network_manager->dns_client, hostname))
        return record->ip;

    if (!network_manager_dns_send_query(network_manager, hostname))
        return 0;

    const uint64_t timeout_time = clock_get_time_since_boot() + DNS_MAX_WAIT;
    while (clock_get_time_since_boot() < timeout_time) {
        if (auto record = dns_client_get_record(network_manager->dns_client, hostname))
            return record->ip;

        vthread_sleep(10);
    }

    return 0;
}

void network_manager_dns_callback(socket_t*, uint32_t ip, uint16_t port, const uint8_t* packet, size_t size) {
    if (size < sizeof(dns_header_t))
        return;

    network_manager_t* network_manager = get_global_network_manager();
    if (!network_manager)
        return;

    const dns_header_t* header = (const dns_header_t*)packet;
    uint16_t ancount = bswap16(header->ancount);
    uint16_t qdcount = bswap16(header->qdcount);

    size_t offset = sizeof(dns_header_t);

    for (int i = 0; i < qdcount && offset < size; i++) {
        size_t name_len = dns_packet_hostname_encoded_length(&packet[offset], size - offset);
        if (name_len == 0)
            return;

        offset += name_len;
        if (offset + 4 > size)
            return;
        offset += 4;
    }

    for (int i = 0; i < ancount && offset < size; i++) {
        char* hostname = dns_packet_decode_hostname(packet, size, offset);

        if (offset + 10 > size) {
            free(hostname);
            return;
        }

        uint16_t qtype = bswap16(*(uint16_t*)&packet[offset]);
        offset += sizeof(uint16_t);

        uint16_t qclass = bswap16(*(uint16_t*)&packet[offset]);
        offset += sizeof(uint16_t);

        uint32_t ttl = bswap32(*(uint32_t*)&packet[offset]);
        offset += sizeof(uint32_t);
        
        uint16_t rdlength = bswap16(*(uint16_t*)&packet[offset]);
        offset += sizeof(uint16_t);

        if (offset + rdlength > size)
            break;

        if (qtype == 1 && rdlength == 4) {
            ipv4_address_t ip = { .raw = bswap32(*(uint32_t*)&packet[offset]) };
            dns_client_add_record(network_manager->dns_client, hostname, ip.raw);
            kprintf("[DNS] stored new record: %s - %u.%u.%u.%u\n", hostname, (uint32_t)ip.byte3, (uint32_t)ip.byte2, (uint32_t)ip.byte1, (uint32_t)ip.byte0);
        }

        offset += rdlength;
    }
}

bool network_manager_configure(network_manager_t* network_manager) {
    network_manager->dhcp_client = dhcp_client_create();
    if (!network_manager->dhcp_client)
        return false;

    network_manager->dhcp_socket = (socket_t*)malloc(sizeof(socket_t));
    memzero(network_manager->dhcp_socket, sizeof(socket_t));
    if (!network_manager->dhcp_socket)
        return false;

    network_manager->dhcp_socket->local_ip = TO_IP(0, 0, 0, 0);
    network_manager->dhcp_socket->local_port = DHCP_PORT_CLIENT;
    network_manager->dhcp_socket->remote_ip = BROADCAST_IPV4;
    network_manager->dhcp_socket->remote_port = DHCP_PORT_SERVER;
    network_manager->dhcp_socket->protocol = socket_protocol_t::UDP;
    network_manager->dhcp_socket->listener = network_manager_dhcp_callback;
    if (!socket_bind(network_manager->dhcp_socket))
        return false;

    network_manager->dns_client = dns_client_create();
    if (!network_manager->dns_client)
        return false;

    network_manager->dns_socket = (socket_t*)malloc(sizeof(socket_t));
    memzero(network_manager->dns_socket, sizeof(socket_t));
    if (!network_manager->dns_socket)
        return false;

    network_manager->dns_socket->local_ip = TO_IP(0, 0, 0, 0);
    network_manager->dns_socket->local_port = random_number(49152, 65535);
    network_manager->dns_socket->remote_ip = DEFAULT_DNS_SERVER;
    network_manager->dns_socket->remote_port = DNS_PORT_SERVER;
    network_manager->dns_socket->protocol = socket_protocol_t::UDP;
    network_manager->dns_socket->listener = network_manager_dns_callback;
    if (!socket_bind(network_manager->dns_socket))
        return false;

    return true;
}