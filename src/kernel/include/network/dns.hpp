//==========================================
/// @file       dns.hpp
/// @brief      
//==========================================

#pragma once

#ifndef __DNS_HPP__
#define __DNS_HPP__

#define DNS_PORT_SERVER 53

#include "common.hpp"
#include "std/map.hpp"

struct dns_header_t {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t ascount;
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
    std::linear_map<uint64_t, dns_cache_record_t> cached_records;
    uint32_t default_server;
    uint32_t default_port;
};

#endif // __DNS_HPP__