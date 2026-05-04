// #define DRIVER_NAMING

// #include "dns.hpp"
// #include "std/random.hpp"
// #include "virtual_reflections_driver.hpp"

// // TODO @since 06/11/2025 -- 15:04
// // header id checks

// uint32_t DNSGenerateHeaderID() {
//     seed_random(KsTimeSinceBoot());
//     return random_number(0, MAX_UINT16);
// }

// std::dynamic_array<uint8_t> DNSQueryBuild(const char* szHostname, DNSQueryType nType) {
//     DNSHeader dnsHeader {};
//     dnsHeader.m_nId = DNSGenerateHeaderID();
//     dnsHeader.m_nFlags = bswap16(0x0100);
//     dnsHeader.m_nQDCount = bswap16(1);
//     dnsHeader.m_nANCount = 0;
//     dnsHeader.m_nNSCount = 0;
//     dnsHeader.m_nARCount = 0;

//     std::dynamic_array<uint8_t> arrEncodedName = DNSEncodeHostname(szHostname);
//     uint16_t nQType = bswap16((uint16_t)nType);
//     uint16_t nQClass = bswap16((uint16_t)1);

//     std::dynamic_array<uint8_t> arrQuery {};
//     arrQuery.resize(sizeof(DNSHeader) + arrEncodedName.length() + 4);

//     // copy header
//     for (size_t i = 0; i < sizeof(DNSHeader); i++)
//         arrQuery.insert_back(((uint8_t*)&dnsHeader)[i]);

//     // copy name
//     for (size_t i = 0; i < arrEncodedName.length(); i++)
//         arrQuery.insert_back(arrEncodedName[i]);

//     // copy type
//     for (size_t i = 0; i < sizeof(uint16_t); i++)
//         arrQuery.insert_back(((uint8_t*)&nQType)[i]);

//     // copy class
//     for (size_t i = 0; i < sizeof(uint16_t); i++)
//         arrQuery.insert_back(((uint8_t*)&nQClass)[i]);

//     return arrQuery;
// }

// std::dynamic_array<uint8_t> DNSEncodeHostname(const char* szHostname) {
//     std::dynamic_array<uint8_t> arrOutput {};
//     arrOutput.resize(strlen(szHostname));

//     std::dynamic_array<std::string> arrParts = str_split(szHostname, '.');

//     for (auto& strPart : arrParts) {
//         arrOutput.insert_back((uint8_t)strPart.length());
//         for (size_t i = 0; i < strPart.length(); i++)
//             arrOutput.insert_back((uint8_t)(strPart.c_str()[i]));
//     }

//     arrOutput.insert_back(0);

//     return arrOutput;
// }

// std::string DNSClientDecodeHostname(const uint8_t* pPacket, size_t nSize, size_t& rOffset) {
//     std::string strResult;
//     bool bJumped = false;
//     size_t oOffset = rOffset;
//     int nMaxLoops = 128;
    
//     while (rOffset < nSize && nMaxLoops-- > 0) {
//         uint8_t nLength = pPacket[rOffset];
        
//         if (nLength == 0) {
//             rOffset++;
//             break;
//         }
        
//         if ((nLength & 0xC0) == 0xC0) {
//             if (rOffset + 1 >= nSize)
//                 break;
            
//             uint16_t pointer = ((nLength & 0x3F) << 8) | pPacket[rOffset + 1];
//             if (pointer >= nSize)
//                 break;
            
//             if (!bJumped) {
//                 oOffset = rOffset + 2;
//                 bJumped = true;
//             }
//             rOffset = pointer;
//             continue;
//         }
        
//         rOffset++;
//         if (rOffset + nLength > nSize)
//             break;
        
//         if (strResult.length() != 0)
//             strResult += '.';
        
//         for (size_t i = 0; i < nLength; i++)
//             strResult += (char)pPacket[rOffset + i];
        
//         rOffset += nLength;
//     }
    
//     if (bJumped)
//         rOffset = oOffset;
    
//     return strResult;
// }

// size_t DNSClientNameLength(const uint8_t* pData, size_t nSize) {
//     size_t nOffset = 0;

//     if (!pData || nSize == 0)
//         return 0;

//     while (nOffset < nSize && pData[nOffset] != 0) {
//         if ((pData[nOffset] & 0xC0) == 0xC0) {
//             if (nOffset + 1 >= nSize)
//                 return 0;
//             nOffset += 2;
//             return nOffset;
//         }

//         uint8_t nLabelLength = pData[nOffset];
//         if (nLabelLength + 1 > nSize - nOffset)
//             return 0;

//         nOffset += nLabelLength + 1;
//     }

//     if (nOffset >= nSize)
//         return 0;

//     return nOffset + 1;
// }

// DNSClientState* DNSClientInit(uint16_t nPort, uint32_t nDNSServerIp) {
//     DNSClientState* pClientState = (DNSClientState*)malloc(sizeof(DNSClientState));
//     memzero(pClientState, sizeof(DNSClientState));

//     pClientState->m_nDNSServerIP = nDNSServerIp;
//     pClientState->m_nPort = nPort;
//     pClientState->m_mapRecords = std::linear_map<std::string, DNSCacheRecord>{};
//     pClientState->m_bIsConfigured = false;

//     return pClientState;
// }

// void DNSClientShutdown(DNSClientState* pClientState) {
//     memzero(pClientState, sizeof(DNSClientState));
//     free(pClientState);
// }

// int DNSClientHandlePacket(DNSClientState* pClientState, uint8_t* pPacket, size_t nSize) {
//     if (nSize < sizeof(DNSHeader))
//         return 1;

//     DNSHeader* dnsHeader = (DNSHeader*)pPacket;
//     uint16_t nANCount = bswap16(dnsHeader->m_nANCount);
//     uint16_t nQDCount = bswap16(dnsHeader->m_nQDCount);

//     size_t nOffset = sizeof(DNSHeader);

//     for (int i = 0; i < nQDCount && nOffset < nSize; i++) {
//         size_t name_len = DNSClientNameLength(&pPacket[nOffset], nSize - nOffset);
//         if (name_len == 0)
//             return 2;

//         nOffset += name_len;
//         if (nOffset + 4 > nSize)
//             return 3;
//         nOffset += 4;
//     }

//     for (int i = 0; i < nANCount && nOffset < nSize; i++) {
//         std::string strHostname = DNSClientDecodeHostname(pPacket, nSize, nOffset);

//         if (nOffset + 10 > nSize)
//             return 1;

//         uint16_t nQType = bswap16(*(uint16_t*)&pPacket[nOffset]);
//         nOffset += sizeof(uint16_t);

//         uint16_t nQClass = bswap16(*(uint16_t*)&pPacket[nOffset]);
//         nOffset += sizeof(uint16_t);

//         uint32_t nTTL = bswap32(*(uint32_t*)&pPacket[nOffset]);
//         nOffset += sizeof(uint32_t);
        
//         uint16_t nRDLength = bswap16(*(uint16_t*)&pPacket[nOffset]);
//         nOffset += sizeof(uint16_t);

//         if (nOffset + nRDLength > nSize)
//             break;

//         if (nQType == 1 && nRDLength == 4) {
//             uint32_t nIP = *(uint32_t*)&pPacket[nOffset];
//             DNSClientSetRecord(pClientState, strHostname.c_str(), bswap32(nIP));
//             KsPrint("stored new dns record");
//         }

//         nOffset += nRDLength;
//     }

//     return 0;
// }

// uint32_t DNSClientResolve(DNSClientState* pClientState, const char* szHostname, DNSSendPacketFN pSendPacket, uint64_t nTimeoutTimeMs) {
//     std::dynamic_array<uint8_t> arrQuery = DNSQueryBuild(szHostname, DNSQueryType::A);

//     if (pSendPacket(pClientState->m_nDNSServerIP, DNS_PORT_SERVER, pClientState->m_nPort, arrQuery.get_data(), arrQuery.length()) != 0)
//         return -1;

//     const uint64_t nTimeoutTime = KsTimeSinceBoot() + nTimeoutTimeMs;
//     const DNSCacheRecord* dnsCacheRecord = nullptr;

//     while (KsTimeSinceBoot() < nTimeoutTime) {
//         dnsCacheRecord = DNSClientGetRecord(pClientState, szHostname);
//         if (dnsCacheRecord)
//             break;

//         KsSleep(10);
//     }

//     if (!dnsCacheRecord) {
//         KsPrint("failed to resolve dns record");
//         return -1;
//     }

//     KsPrint("resolved dns record");

//     return dnsCacheRecord->m_nIP;
// }

// const DNSCacheRecord* DNSClientGetRecord(DNSClientState* pClientState, const char* szHostname) {
//     if (auto it = pClientState->m_mapRecords.get(szHostname); it != pClientState->m_mapRecords.end())
//         return &it->value;

//     return nullptr;
// }

// void DNSClientSetRecord(DNSClientState* pClientState, const char* szHostname, uint32_t nIP) {
//     pClientState->m_mapRecords[szHostname] = DNSCacheRecord{ .m_strHostname = szHostname, .m_nIP = nIP };
// }