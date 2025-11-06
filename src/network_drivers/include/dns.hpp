//==========================================
/// @file       dns.hpp
/// @brief      dns driver version
//==========================================

#pragma once

#ifndef __DNS_HPP__
#define __DNS_HPP__

#define DNS_PORT_SERVER 53

#include "common.hpp"
#include "std/string.hpp"
#include "std/map.hpp"

struct DNSHeader {
    uint16_t m_nId;
    uint16_t m_nFlags;
    uint16_t m_nQDCount;
    uint16_t m_nANCount;
    uint16_t m_nNSCount;
    uint16_t m_nARCount;
} PACKED;

struct DNSQuery {
    uint16_t m_nQType;
    uint16_t m_nQClass;
} PACKED;

struct DNSRecord {
    uint16_t m_nQType;
    uint16_t m_nQClass;
    uint32_t m_nTTL;
    uint16_t m_nRDLength;
} PACKED;

enum class DNSQueryType : uint16_t {
    A = 1,
    NS = 2,
    CNAME = 5,
    MX = 15,
    AAAA = 28
};

struct DNSCacheRecord {
    std::string m_strHostname;
    uint32_t m_nIP;
};

struct DNSClientState {
    std::linear_map<std::string, DNSCacheRecord> m_mapRecords;
    uint32_t m_nDNSServerIP;
    uint16_t m_nPort;
    bool m_bIsConfigured;
};

typedef int(*DNSSendPacketFN)(uint32_t nDstIp, uint16_t nDstPort, uint16_t nSrcPort, uint8_t* pData, size_t nSize);

uint32_t DNSGenerateHeaderID();

std::dynamic_array<uint8_t> DNSQueryBuild(const char* szHostname, DNSQueryType nType);
std::dynamic_array<uint8_t> DNSEncodeHostname(const char* szHostname);

std::string DNSClientDecodeHostname(const uint8_t* pPacket, size_t nSize, size_t& rOffset);
size_t DNSClientNameLength(const uint8_t* pData, size_t nSize);

/// @brief dns exports
extern "C" {
    DNSClientState* DNSClientInit(uint16_t nPort, uint32_t nDNSServerIp);
    void DNSClientShutdown(DNSClientState* pClientState);

    int DNSClientHandlePacket(DNSClientState* pClientState, uint8_t* pPacket, size_t nSize);
    
    uint32_t DNSClientResolve(DNSClientState* pClientState, const char* szHostname, DNSSendPacketFN pSendPacket, uint64_t nTimeoutTimeMs = 1000);
    const DNSCacheRecord* DNSClientGetRecord(DNSClientState* pClientState, const char* szHostname);
    void DNSClientSetRecord(DNSClientState* pClientState, const char* szHostname, uint32_t nIP);
}

#endif // __DNS_HPP__