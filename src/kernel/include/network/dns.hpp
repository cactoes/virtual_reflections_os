//==========================================
/// @file       dns.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DNS_HPP__
#define __DNS_HPP__

#define DNS_PORT_SERVER 53
#define DNS_MAX_NAME_LENGTH 255

#include "common.hpp"
#include "std/map.hpp"
#include "utils/mutex.hpp"

struct dns_header_t {
    u16 id;
    u16 flags;
    u16 qdcount;
    u16 ancount;
    u16 nscount;
    u16 arcount;
} PACKED;

struct dns_query_t {
    u16 qtype;
    u16 qclass;
} PACKED;

struct dns_record_t {
    u16 qtype;
    u16 qclass;
    u16 ttl;
    u16 rdlength;
} PACKED;

enum class dns_query_type_t : u16 {
    A = 1,
    NS = 2,
    CNAME = 5,
    MX = 15,
    AAAA = 28
};

struct dns_cache_record_t {
    u32 ip;
    char* hostname;
};

struct dns_client_t {
    mutex_t mutex;
    std::linear_map<u64, dns_cache_record_t> cached_records;
};

u8* dns_create_query_packet(const char* hostname, dns_query_type_t type, size_t* size);
char* dns_packet_decode_hostname(const u8* src, size_t size, size_t& offset);
size_t dns_packet_hostname_encoded_length(const u8* src, size_t size);

dns_client_t* dns_client_create();
bool dns_client_destroy(dns_client_t* client);
bool dns_client_add_record(dns_client_t* client, char* hostname, u32 ip);
const dns_cache_record_t* dns_client_get_record(dns_client_t* client, const char* hostname);

#endif // __DNS_HPP__