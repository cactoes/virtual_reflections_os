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
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} PACKED;

struct dns_query_t {
    uint16_t qtype;
    uint16_t qclass;
} PACKED;

struct dns_record_t {
    uint16_t qtype;
    uint16_t qclass;
    uint16_t ttl;
    uint16_t rdlength;
} PACKED;

enum class dns_query_type_t : uint16_t {
    A = 1,
    NS = 2,
    CNAME = 5,
    MX = 15,
    AAAA = 28
};

struct dns_cache_record_t {
    uint32_t ip;
    char* hostname;
};

struct dns_client_t {
    mutex_t mutex;
    std::linear_map<uint64_t, dns_cache_record_t> cached_records;
};

uint8_t* dns_create_query_packet(const char* hostname, dns_query_type_t type, size_t* size);
char* dns_packet_decode_hostname(const uint8_t* src, size_t size, size_t& offset);
size_t dns_packet_hostname_encoded_length(const uint8_t* src, size_t size);

dns_client_t* dns_client_create();
bool dns_client_destroy(dns_client_t* client);
bool dns_client_add_record(dns_client_t* client, char* hostname, uint32_t ip);
const dns_cache_record_t* dns_client_get_record(dns_client_t* client, const char* hostname);

#endif // __DNS_HPP__