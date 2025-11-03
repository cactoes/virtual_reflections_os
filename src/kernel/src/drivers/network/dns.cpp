#include "drivers/network/dns.hpp"
#include "std/random.hpp"
#include "utils/map.hpp"
#include "io.hpp"
#include "drivers/network/nidm.hpp"

// void dns_cache_records_insert(const string& hostname, uint32_t ip) {
//     global_dns_cache_records[hostname] = dns_cache_record_t{ .name = hostname, .ip = ip };
// }

// const dns_cache_record_t* dns_cache_records_get(const string& hostname) {
//     if (auto it = global_dns_cache_records.get(hostname); it != global_dns_cache_records.end())
//         return &it->value;

//     return nullptr;
// }

static dns_client_t* global_dns_client = nullptr;

void set_global_dns_client(dns_client_t* client) {
    global_dns_client = client;
}

dns_client_t* get_global_dns_client() {
    return global_dns_client;
}

void dns_client_init(dns_client_t* client) {
    client->records = linear_map<string, dns_cache_record_t>{};
    client->port = random_number(49152, 65535);
}

void dns_client_store_record(dns_client_t* client, const string& hostname, uint32_t ip) {
    client->records[hostname] = dns_cache_record_t{ .name = hostname, .ip = ip };
}

const dns_cache_record_t* dns_client_get_record(dns_client_t* client, const string& hostname, uint32_t ip) {
    if (auto it = client->records.get(hostname); it != client->records.end())
        return &it->value;

    return nullptr;
}

std::dynamic_array<uint8_t> dns_encode_hostname(const string& hostname) {
    std::dynamic_array<uint8_t> output {};
    output.resize(hostname.length());

    std::dynamic_array<string> parts = str_split(hostname, '.');

    for (auto& part : parts) {
        output.insert_back((uint8_t)part.length());
        for (size_t i = 0; i < part.length(); i++) {
            output.insert_back((uint8_t)(part.c_str()[i]));
        }
    }

    output.insert_back(0);

    return output;
}

string dns_decode_hostname(const uint8_t* bytes, size_t size) {
    string result;
    size_t offset = 0;
    
    while (offset < size && bytes[offset] != 0) {
        uint8_t length = bytes[offset];
        offset++;
        
        // Check bounds
        if (offset + length > size) {
            break;
        }
        
        // Add dot separator (except for first label)
        if (result.length() != 0) {
            result += ".";
        }
        
        // Append the label
        for (size_t i = 0; i < length; i++) {
            result += (char)bytes[offset + i];
        }
        
        offset += length;
    }
    
    return result;
}

std::dynamic_array<uint8_t> dns_build_query(const string& hostname, dns_query_type_t type) {
    dns_header_t header {};
    header.id = host_to_net<uint16_t>(random_number(0, MAX_UINT16));
    header.flags = host_to_net<uint16_t>(0x0100);
    header.qdcount = host_to_net<uint16_t>(1);
    header.ancount = 0;
    header.nscount = 0;
    header.arcount = 0;

    std::dynamic_array<uint8_t> encoded_name = dns_encode_hostname(hostname);
    uint16_t qtype = host_to_net((uint16_t)type);
    uint16_t qclass = host_to_net((uint16_t)1);

    std::dynamic_array<uint8_t> query {};
    query.resize(sizeof(dns_header_t) + encoded_name.length() + 4);
    
    // copy header
    for (size_t i = 0; i < sizeof(dns_header_t); i++)
        query.insert_back(((uint8_t*)&header)[i]);

    // copy name
    for (size_t i = 0; i < encoded_name.length(); i++)
        query.insert_back(encoded_name[i]);

    // copy type
    for (size_t i = 0; i < sizeof(uint16_t); i++)
        query.insert_back(((uint8_t*)&qtype)[i]);

    // copy class
    for (size_t i = 0; i < sizeof(uint16_t); i++)
        query.insert_back(((uint8_t*)&qclass)[i]);

    return query;
}

size_t dns_name_length(const uint8_t* data, size_t size) {
    size_t offset = 0;

    if (!data || size == 0)
        return 0;

    while (offset < size && data[offset] != 0) {
        if ((data[offset] & 0xC0) == 0xC0) {
            if (offset + 1 >= size)
                return 0;
            offset += 2;
            return offset;
        }

        uint8_t label_len = data[offset];
        if (label_len + 1 > size - offset)
            return 0;

        offset += label_len + 1;
    }

    if (offset >= size)
        return 0;

    return offset + 1;
}

int dns_receive(const uint8_t* packet, size_t size) {
    if (size < sizeof(dns_header_t))
        return 1;

    dns_header_t* header = (dns_header_t*)packet;
    
    uint16_t ancount = net_to_host(header->ancount);
    uint16_t qdcount = net_to_host(header->qdcount);

    size_t offset = sizeof(dns_header_t);

    // skip questions
    for (int i = 0; i < qdcount && offset < size; i++) {
        size_t name_len = dns_name_length(&packet[offset], size - offset);
        if (name_len == 0)
            return 2;

        offset += name_len;
        if (offset + 4 > size)
            return 3;
        offset += 4;
    }


    for (int i = 0; i < ancount && offset < size; i++) {
        string hostname = dns_decode_hostname(&packet[offset], size - offset);
        
        size_t name_len = dns_name_length(&packet[offset], size - offset);
        if (name_len == 0 || offset + name_len > size)
            return 1;

        offset += name_len;

        if (offset + 10 > size)
            return 1;

        uint16_t type = net_to_host(*(uint16_t*)&packet[offset]);
        offset += sizeof(uint16_t);

        uint16_t class_ = net_to_host(*(uint16_t*)&packet[offset]);
        offset += sizeof(uint16_t);

        uint32_t ttl = net_to_host(*(uint32_t*)&packet[offset]);
        offset += sizeof(uint32_t);
        
        uint16_t rdlength = net_to_host(*(uint16_t*)&packet[offset]);
        offset += sizeof(uint16_t);

        if (offset + rdlength > size)
            break;

        if (type == 1 && rdlength == 4) {
            uint32_t ip = *(uint32_t*)&packet[offset];
            kprintf("%s -> %u.%u.%u.%u (TTL: %u)\n", 
                   hostname.c_str(),
                   packet[offset], packet[offset+1], 
                   packet[offset+2], packet[offset+3],
                   ttl);

            dns_client_store_record(get_global_dns_client(), hostname, ip);
        }

        offset += rdlength;
    }

    return 0;
}

void dns_client_start() {
    auto client = get_global_dns_client();

    auto net_recieve = [](uint8_t* p, size_t s) { dns_receive(p, s); };
    nidm_udp_bind(get_global_nidm(), client->port, net_recieve);
}

int dns_client_thread() {
    dns_client_t client {};
    dns_client_init(&client);
    set_global_dns_client(&client);

    dns_client_start();

    while (true);
    return 0;
}