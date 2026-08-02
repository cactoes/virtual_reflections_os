// //==========================================
// /// @file       dns.hpp
// /// @brief      dns driver version
// //==========================================

// #pragma once

// #ifndef __DNS_HPP__
// #define __DNS_HPP__

// #define DNS_PORT_SERVER 53

// #include "common.hpp"
// #include "std/string.hpp"
// #include "std/map.hpp"

// struct DNSHeader {
//     u16 m_nId;
//     u16 m_nFlags;
//     u16 m_nQDCount;
//     u16 m_nANCount;
//     u16 m_nNSCount;
//     u16 m_nARCount;
// } __packed;

// struct DNSQuery {
//     u16 m_nQType;
//     u16 m_nQClass;
// } __packed;

// struct DNSRecord {
//     u16 m_nQType;
//     u16 m_nQClass;
//     u32 m_nTTL;
//     u16 m_nRDLength;
// } __packed;

// enum class DNSQueryType : u16 {
//     A = 1,
//     NS = 2,
//     CNAME = 5,
//     MX = 15,
//     AAAA = 28
// };

// struct DNSCacheRecord {
//     std::string m_strHostname;
//     u32 m_nIP;
// };

// struct DNSClientState {
//     std::linear_map<std::string, DNSCacheRecord> m_mapRecords;
//     u32 m_nDNSServerIP;
//     u16 m_nPort;
//     bool m_bIsConfigured;
// };

// typedef int(*DNSSendPacketFN)(u32 nDstIp, u16 nDstPort, u16 nSrcPort, u8* pData, size_t nSize);

// u32 DNSGenerateHeaderID();

// std::dynamic_array<u8> DNSQueryBuild(const char* szHostname, DNSQueryType nType);
// std::dynamic_array<u8> DNSEncodeHostname(const char* szHostname);

// std::string DNSClientDecodeHostname(const u8* pPacket, size_t nSize, size_t& rOffset);
// size_t DNSClientNameLength(const u8* pData, size_t nSize);

// /// @brief dns exports
// extern "C" {
//     DNSClientState* DNSClientInit(u16 nPort, u32 nDNSServerIp);
//     void DNSClientShutdown(DNSClientState* pClientState);

//     int DNSClientHandlePacket(DNSClientState* pClientState, u8* pPacket, size_t nSize);
    
//     u32 DNSClientResolve(DNSClientState* pClientState, const char* szHostname, DNSSendPacketFN pSendPacket, u64 nTimeoutTimeMs = 1000);
//     const DNSCacheRecord* DNSClientGetRecord(DNSClientState* pClientState, const char* szHostname);
//     void DNSClientSetRecord(DNSClientState* pClientState, const char* szHostname, u32 nIP);
// }

// #endif // __DNS_HPP__