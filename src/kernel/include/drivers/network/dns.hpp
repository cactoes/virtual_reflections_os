//==========================================
/// @file       dns.hpp
/// @brief      domain name s...
//==========================================

#pragma once

#ifndef __DRIVERS_NETWORK_DNS_HPP__
#define __DRIVERS_NETWORK_DNS_HPP__

#include "../../../../network_drivers/include/dns.hpp"
#include "common.hpp"
// #include "std/string.hpp"
// #include "std/array.hpp"
// #include "std/map.hpp"
// #include "utils/mutex.hpp"


// struct dns_header_t {
//     uint16_t id;
//     uint16_t flags;
//     uint16_t qdcount;
//     uint16_t ancount;
//     uint16_t nscount;
//     uint16_t arcount;
// } PACKED;

// struct dns_query_t {
//     uint16_t qtype;
//     uint16_t qclass;
// } PACKED;

// struct dns_record_t {
//     uint16_t type;
//     uint16_t qclass;
//     uint32_t ttl;
//     uint16_t rdlength;
// } PACKED;

// enum class dns_query_type_t {
//     A = 1,
//     NS = 2,
//     CNAME = 5,
//     MX = 15,
//     AAAA = 28
// };

// struct dns_cache_record_t {
//     std::string name;
//     uint32_t ip;
// };

// struct dns_client_t {
//     std::linear_map<std::string, dns_cache_record_t> records;
//     mutex_t mutex;
//     uint16_t port;
//     bool is_configured;
//     uint32_t dns_server;
// };

// void set_global_dns_client(dns_client_t* client);
// dns_client_t* get_global_dns_client();

// void dns_client_init(dns_client_t* client);

// void dns_client_store_record(dns_client_t* client, const std::string& hostname, uint32_t ip);
// const dns_cache_record_t* dns_client_get_record(dns_client_t* client, const std::string& hostname);
// uint32_t dns_client_query(dns_client_t* client, const std::string& hostname);
// int dns_client_thread();
// bool dns_client_is_configured(dns_client_t* client);

#endif // __DRIVERS_NETWORK_DNS_HPP__