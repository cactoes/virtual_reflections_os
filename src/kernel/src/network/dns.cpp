#include "network/dns.hpp"
#include "std/random.hpp"
#include "std/string.hpp"
#include "time/clock.hpp"

u16 dns_generate_header_id() {
    seed_random(clock_get_time_since_boot());
    return (u16)random_number(0, MAX_UINT16);
}

u8* dns_encode_hostname(const char* hostname, size_t* size) {
    if (!hostname || !size)
        return nullptr;

    size_t hostname_length = strlen(hostname);
    u8* encoded_string = (u8*)malloc(DNS_MAX_NAME_LENGTH + 1);
    if (!encoded_string)
        return nullptr;

    size_t write_ptr = 0;
    size_t length = 0;
    const char* start = hostname;

    for (size_t i = 0; i <= hostname_length; i++) {
        if (hostname[i] == '.' || hostname[i] == '\0') {
            if (length == 0)
                break;

            if (length > 63) {
                free(encoded_string);
                return nullptr;
            }

            encoded_string[write_ptr++] = (u8)length;
            memcpy(&encoded_string[write_ptr], start, length);
            write_ptr += length;

            if (hostname[i] == '\0')
                break;

            start += length + 1;
            length = 0;
            continue;
        }

        length++;
    }

    encoded_string[write_ptr++] = 0;

    if (write_ptr > 255) {
        free(encoded_string);
        return nullptr;
    }

    *size = write_ptr;
    return encoded_string;
}

u8* dns_create_query_packet(const char* hostname, dns_query_type_t type, size_t* size) {
    if (!hostname || !size)
        return nullptr;

    dns_header_t header {};
    memzero(&header, sizeof(header));

    header.id = dns_generate_header_id();
    header.flags = bswap16(0x0100);
    header.qdcount = bswap16(1);

    size_t encoded_hostname_length = 0;
    u8* encoded_hostname = dns_encode_hostname(hostname, &encoded_hostname_length);
    if (!encoded_hostname)
        return nullptr;

    dns_query_t query {};
    query.qtype = bswap16((u16)type);
    query.qclass = bswap16(1);

    size_t dns_query_packet_length = sizeof(dns_header_t) + encoded_hostname_length + sizeof(dns_query_t);
    u8* dns_query_packet = (u8*)malloc(dns_query_packet_length);
    if (!dns_query_packet) {
        free(encoded_hostname);
        return nullptr;
    }

    memcpy(dns_query_packet, (u8*)&header, sizeof(dns_header_t));
    memcpy(dns_query_packet + sizeof(dns_header_t), encoded_hostname, encoded_hostname_length);
    memcpy(dns_query_packet + sizeof(dns_header_t) + encoded_hostname_length, (u8*)&query, sizeof(dns_query_t));

    free(encoded_hostname);

    *size = dns_query_packet_length;
    return dns_query_packet;
}

char* dns_packet_decode_hostname(const u8* src, size_t size, size_t& offset) {
    if (!src || offset >= size)
        return nullptr;

    char* out = (char*)malloc(DNS_MAX_NAME_LENGTH + 1);
    if (!out)
        return nullptr;

    size_t out_len = 0;

    bool visited[DNS_MAX_NAME_LENGTH];
    memzero(visited, sizeof(visited));

    bool jumped = false;
    size_t original_offset = offset;

    size_t loops = DNS_MAX_NAME_LENGTH;

    while (offset < size && loops-- > 0) {
        u8 len = src[offset];

        if (len == 0) {
            offset++;
            break;
        }

        u8 type = len & 0xC0;

        if (type == 0xC0) {
            if (offset + 1 >= size) {
                free(out);
                return nullptr;
            }

            u16 ptr =
                ((u16)(len & 0x3F) << 8) |
                src[offset + 1];

            if (ptr >= size) {
                free(out);
                return nullptr;
            }

            if (ptr < DNS_MAX_NAME_LENGTH) {
                if (visited[ptr]) {
                    free(out);
                    return nullptr;
                }
                visited[ptr] = true;
            }

            if (!jumped) {
                original_offset = offset + 2;
                jumped = true;
            }

            offset = ptr;
            continue;
        }

        if (type != 0x00) {
            free(out);
            return nullptr;
        }

        if (len > 63) {
            free(out);
            return nullptr;
        }
        offset++;

        if (offset + len > size) {
            free(out);
            return nullptr;
        }

        if (out_len != 0) {
            if (out_len + 1 >= DNS_MAX_NAME_LENGTH) {
                free(out);
                return nullptr;
            }
            out[out_len++] = '.';
        }

        if (out_len + len >= DNS_MAX_NAME_LENGTH) {
            free(out);
            return nullptr;
        }
        memcpy(out + out_len, src + offset, len);

        out_len += len;
        offset += len;
    }

    if (loops == 0) {
        free(out);
        return nullptr;
    }
    out[out_len] = '\0';

    if (jumped)
        offset = original_offset;

    return out;
}

size_t dns_packet_hostname_encoded_length(const u8* src, size_t size) {
    if (!src || size == 0)
        return 0;

    size_t offset = 0;
    size_t loops = DNS_MAX_NAME_LENGTH;

    while (offset < size && loops-- > 0) {
        u8 len = src[offset];

        if (len == 0)
            return offset + 1;

        u8 type = len & 0xC0;

        if (type == 0xC0) {
            if (offset + 1 >= size)
                return 0;

            u16 ptr = ((u16)(len & 0x3F) << 8) | src[offset + 1];

            if (ptr >= size)
                return 0;

            return offset + 2;
        }

        if (type != 0x00)
            return 0;

        if (len > 63)
            return 0;

        offset++;

        if (offset + len > size)
            return 0;

        offset += len;
    }

    return 0;
}

dns_client_t* dns_client_create() {
    dns_client_t* client = new dns_client_t {};
    mutex_init(&client->mutex);
    client->cached_records = std::linear_map<u64, dns_cache_record_t>{};
    return client;
}

bool dns_client_destroy(dns_client_t* client) {
    if (!client)
        return false;

    delete client;

    return true;
}

bool dns_client_add_record(dns_client_t* client, char* hostname, u32 ip) {
    if (!client || !hostname || ip == 0)
        return false;

    mutex_lock_guard guard(&client->mutex);
    client->cached_records.insert(hash_fnv1a_64(hostname), dns_cache_record_t{
        .ip = ip,
        .hostname = hostname
    });
    return true;
}

const dns_cache_record_t* dns_client_get_record(dns_client_t* client, const char* hostname) {
    if (!client || !hostname)
        return nullptr;

    mutex_lock_guard guard(&client->mutex);
    auto it = client->cached_records.get(hash_fnv1a_64(hostname));
    if (it == client->cached_records.end())
        return nullptr;

    return &it->value;
}

// void dns_client_init(dns_client_t* client) {
//     client->records = std::linear_map<std::string, dns_cache_record_t>{};
//     client->port = random_number(49152, 65535);
//     client->is_configured = false;
//     client->mutex = mutex_t{};
//     mutex_init(&client->mutex);
//     client->dns_server = TO_IP(8, 8, 8, 8);
// }

// void dns_client_store_record(dns_client_t* client, const std::string& hostname, u32 ip) {
//     mutex_lock_guard guard(&client->mutex);

//     client->records[hostname] = dns_cache_record_t{ .name = hostname, .ip = ip };
// }

// const dns_cache_record_t* dns_client_get_record(dns_client_t* client, const std::string& hostname) {
//     mutex_lock_guard guard(&client->mutex);

//     if (auto it = client->records.get(hostname); it != client->records.end())
//         return &it->value;

//     return nullptr;
// }

// std::string dns_decode_hostname(const u8* packet, size_t packet_size, size_t& offset) {
//     std::string result;
//     bool jumped = false;
//     size_t original_offset = offset;
//     int max_loops = 128;
    
//     while (offset < packet_size && max_loops-- > 0) {
//         u8 len = packet[offset];
        
//         if (len == 0) {
//             offset++;
//             break;
//         }
        
//         if ((len & 0xC0) == 0xC0) {
//             if (offset + 1 >= packet_size)
//                 break;
            
//             u16 pointer = ((len & 0x3F) << 8) | packet[offset + 1];
//             if (pointer >= packet_size)
//                 break;
            
//             if (!jumped) {
//                 original_offset = offset + 2;
//                 jumped = true;
//             }
//             offset = pointer;
//             continue;
//         }
        
//         offset++;
//         if (offset + len > packet_size)
//             break;
        
//         if (result.length() != 0)
//             result += '.';
        
//         for (size_t i = 0; i < len; i++)
//             result += (char)packet[offset + i];
        
//         offset += len;
//     }
    
//     if (jumped)
//         offset = original_offset;
    
//     return result;
// }

// std::dynamic_array<u8> dns_build_query(const std::string& hostname, dns_query_type_t type) {
//     dns_header_t header {};
//     header.id = bswap16(random_number(0, MAX_UINT16));
//     header.flags = bswap16(0x0100);
//     header.qdcount = bswap16(1);
//     header.ancount = 0;
//     header.nscount = 0;
//     header.arcount = 0;

//     std::dynamic_array<u8> encoded_name = dns_encode_hostname(hostname);
//     u16 qtype = host_to_net((u16)type);
//     u16 qclass = host_to_net((u16)1);

//     std::dynamic_array<u8> query {};
//     query.resize(sizeof(dns_header_t) + encoded_name.length() + 4);
    
//     // copy header
//     for (size_t i = 0; i < sizeof(dns_header_t); i++)
//         query.insert_back(((u8*)&header)[i]);

//     // copy name
//     for (size_t i = 0; i < encoded_name.length(); i++)
//         query.insert_back(encoded_name[i]);

//     // copy type
//     for (size_t i = 0; i < sizeof(u16); i++)
//         query.insert_back(((u8*)&qtype)[i]);

//     // copy class
//     for (size_t i = 0; i < sizeof(u16); i++)
//         query.insert_back(((u8*)&qclass)[i]);

//     return query;
// }

// size_t dns_name_length(const u8* data, size_t size) {
//     size_t offset = 0;

//     if (!data || size == 0)
//         return 0;

//     while (offset < size && data[offset] != 0) {
//         if ((data[offset] & 0xC0) == 0xC0) {
//             if (offset + 1 >= size)
//                 return 0;
//             offset += 2;
//             return offset;
//         }

//         u8 label_len = data[offset];
//         if (label_len + 1 > size - offset)
//             return 0;

//         offset += label_len + 1;
//     }

//     if (offset >= size)
//         return 0;

//     return offset + 1;
// }

// int dns_receive(const u8* packet, size_t size) {
//     if (size < sizeof(dns_header_t))
//         return 1;

//     dns_header_t* header = (dns_header_t*)packet;
    
//     u16 ancount = net_to_host(header->ancount);
//     u16 qdcount = net_to_host(header->qdcount);

//     size_t offset = sizeof(dns_header_t);

//     for (int i = 0; i < qdcount && offset < size; i++) {
//         size_t name_len = dns_name_length(&packet[offset], size - offset);
//         if (name_len == 0)
//             return 2;

//         offset += name_len;
//         if (offset + 4 > size)
//             return 3;
//         offset += 4;
//     }

//     for (int i = 0; i < ancount && offset < size; i++) {
//         std::string hostname = dns_decode_hostname(packet, size, offset);

//         if (offset + 10 > size)
//             return 1;

//         u16 type = net_to_host(*(u16*)&packet[offset]);
//         offset += sizeof(u16);

//         u16 class_ = net_to_host(*(u16*)&packet[offset]);
//         offset += sizeof(u16);

//         u32 ttl = net_to_host(*(u32*)&packet[offset]);
//         offset += sizeof(u32);
        
//         u16 rdlength = net_to_host(*(u16*)&packet[offset]);
//         offset += sizeof(u16);

//         if (offset + rdlength > size)
//             break;

//         if (type == 1 && rdlength == 4) {
//             u32 ip = *(u32*)&packet[offset];
//             kprintf("2. %s -> %u.%u.%u.%u (TTL: %u)\n", 
//                    hostname.c_str(),
//                    packet[offset], packet[offset+1], 
//                    packet[offset+2], packet[offset+3],
//                    ttl);

//             dns_client_store_record(get_global_dns_client(), hostname, net_to_host(ip));
//         }

//         offset += rdlength;
//     }

//     return 0;
// }

// void net_recieve(u8* p, size_t s) { dns_receive(p, s); };

// void dns_client_start() {
//     auto client = get_global_dns_client();

//     nidm_udp_bind(get_global_nidm(), client->port, net_recieve);

//     client->is_configured = true;
// }

// int dns_client_thread() {
//     dns_client_t client {};
//     dns_client_init(&client);
//     set_global_dns_client(&client);

//     dns_client_start();

//     while (true);
//     return 0;
// }

// u32 dns_client_query(dns_client_t* client, const std::string& hostname) {
//     auto query = dns_build_query(hostname, dns_query_type_t::A);

//     if (udp_send(client->dns_server, client->port, DNS_SERVER_PORT, query.get_data(), query.length()) != 0)
//         return -1;

//     const u64 timeout_time = clock_get_time_since_boot() + 1000;
//     const dns_cache_record_t* record = nullptr;

//     while (clock_get_time_since_boot() < timeout_time) {
//         record = dns_client_get_record(client, hostname);
//         if (record)
//             break;

//         vthread_sleep(10);
//     }

//     if (!record)
//         return -1;

//     return record->ip;
// }

// bool dns_client_is_configured(dns_client_t* client) {
//     if (!client)
//         return false;

//     return client->is_configured;
// }