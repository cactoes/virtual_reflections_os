//==========================================
/// @file       dhcp.hpp
/// @brief      driver dhcp version
//==========================================

#pragma once

#ifndef __DHCP_HPP__
#define __DHCP_HPP__

#define DHCP_PORT_CLIENT                        68
#define DHCP_PORT_SERVER                        67

#define DHCP_MAGIC                              (uint32_t)0x63825363

#define DHCP_OP_BOOTREQUEST                     1
#define DHCP_OP_BOOTREPLY                       2

#define DHCP_HTYPE_ETHERNET                     1
#define DHCP_HLEN_ETHERNET_ADDRESS              6

#define DHCP_MESSAGE_TYPE_DHCPDISCOVER          1
#define DHCP_MESSAGE_TYPE_DHCPOFFER             2
#define DHCP_MESSAGE_TYPE_DHCPREQUEST           3
#define DHCP_MESSAGE_TYPE_DHCPDECLINE           4
#define DHCP_MESSAGE_TYPE_DHCPACK               5
#define DHCP_MESSAGE_TYPE_DHCPNAK               6
#define DHCP_MESSAGE_TYPE_DHCPRELEASE           7
#define DHCP_MESSAGE_TYPE_DHCPINFORM            8
#define DHCP_MESSAGE_TYPE_DHCPFORCERENEW        9
#define DHCP_MESSAGE_TYPE_DHCPLEASEQUERY        10
#define DHCP_MESSAGE_TYPE_DHCPLEASEUNASSIGNED   11
#define DHCP_MESSAGE_TYPE_DHCPLEASEUNKNOWN      12
#define DHCP_MESSAGE_TYPE_DHCPLEASEACTIVE       13

#define DHCP_OPTION_SUBNET_MASK                 1
#define DHCP_OPTION_ROUTER                      3
#define DHCP_OPTION_DNS                         6
#define DHCP_OPTION_HOSTNAME                    12
#define DHCP_OPTION_REQUESTED_IP_ADDR           50
#define DHCP_OPTION_IP_LEASE_TIME               51
#define DHCP_OPTION_DHCP_MESSAGE_TYPE           53
#define DHCP_OPTION_DHCP_SERVER_ID              54
#define DHCP_OPTION_CLIENT_ID                   61
#define DHCP_OPTION_END                         255

#define DHCP_CLIENT_RECIEVE_ERR                 1
#define DHCP_CLIENT_RECIEVE_ACK                 2
#define DHCP_CLIENT_RECIEVE_OTH                 3
#define DHCP_CLIENT_RECIEVE_REQ                 4

#include "common.hpp"
#include "std/pointer.hpp"

struct DHCPPacket {
    uint8_t m_nOp;
    uint8_t m_nHType;
    uint8_t m_nHLen;
    uint8_t m_nHops;
    uint32_t m_nXID;
    uint16_t m_nSecs;
    uint16_t m_nFlags;

    uint32_t m_nClientIPAddress;
    uint32_t m_nYourIPAddress;
    uint32_t m_nServerIPAddress;
    uint32_t m_nGatewayIPAddress;

    uint8_t m_aClientHWAddress[16];
    uint8_t m_aServerName[64];
    uint8_t m_aFile[128];

    uint32_t m_nMagic;

    uint8_t m_aOptions[308];
} PACKED;

struct DHCPOptionsWriter {
    uint8_t* m_pBuffer;
    size_t m_nBufferSize;
    size_t m_nOffset;
};

template <size_t size>
struct DHCPOption {
    uint8_t m_nType;
    uint8_t m_nLength;
    uint8_t m_aValue[size];
};

void DHCPOptionsWriterInit(DHCPOptionsWriter* pWriter, uint8_t* pBuffer, size_t nBufferSize);
bool DHCPOptionsWriterAddOption(DHCPOptionsWriter* pWriter, uint8_t nType, uint8_t* pData, size_t nLength);
bool DHCPOptionsWriterShutdown(DHCPOptionsWriter* pWriter);

extern "C" uint32_t DHCPGenerateXID();

extern "C" uint8_t* DHCPGetOption(DHCPPacket* pPacket, uint8_t nType);
extern "C" DHCPPacket DHCPCreateDiscoverPacket(const char* szHostName, uint32_t nXID, uint8_t pMac[6]);
extern "C" DHCPPacket DHCPCreateRequestPacket(const char* szHostName, uint32_t nWantedIP, uint32_t nDHCPServerIP, uint32_t nXID, uint8_t pMac[6]);

extern "C" uint32_t DHCPClientDiscover(const char* szHostName, uint8_t pMac[6]);
extern "C" int DHCPClientRecieve(DHCPPacket* pPacket, uint32_t nXID, uint32_t* pIP, uint32_t* pSubnetMask, uint32_t* pGatewayIp, uint32_t* pDHCPServerID);

#endif // __DHCP_HPP__