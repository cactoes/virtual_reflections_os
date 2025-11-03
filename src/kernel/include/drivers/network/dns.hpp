//==========================================
/// @file       dns.hpp
/// @brief      domain name s...
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_DNS_HPP__
#define __DRIVERS_NETWORK_DNS_HPP__

#include "common.hpp"
#include "string.hpp"
#include "std/array.hpp"

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
    uint16_t type;
    uint16_t qclass;
    uint32_t ttl;
    uint16_t rdlength;
} PACKED;

enum class dns_query_type_t {
    A = 1,
    NS = 2,
    CNAME = 5,
    MX = 15,
    AAAA = 28
};

struct dns_cache_record_t {
    string name;
    uint32_t ip;
};

struct dns_client_t {
    linear_map<string, dns_cache_record_t> records;
    uint16_t port;
};

void set_global_dns_client(dns_client_t* client);
dns_client_t* get_global_dns_client();

void dns_client_init(dns_client_t* client);

void dns_client_store_record(dns_client_t* client, const string& hostname, uint32_t ip);
const dns_cache_record_t* dns_client_get_record(dns_client_t* client, const string& hostname, uint32_t ip);

std::dynamic_array<uint8_t> dns_encode_hostname(const string& hostname);
string dns_decode_hostname(const uint8_t* bytes, size_t size);

std::dynamic_array<uint8_t> dns_build_query(const string& hostname, dns_query_type_t type);
int dns_receive(const uint8_t* packet, size_t size);

#endif // __DRIVERS_NETWORK_DNS_HPP__