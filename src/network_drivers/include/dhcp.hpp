//==========================================
/// @file       dhcp.hpp
/// @brief      driver dhcp version
//==========================================

#pragma once

#ifndef __DHCP_HPP__
#define __DHCP_HPP__

#define DHCP_PORT_CLIENT                        68
#define DHCP_PORT_SERVER                        67

#define DHCP_MAGIC                              (u32)0x63825363

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
    u8 m_nOp;
    u8 m_nHType;
    u8 m_nHLen;
    u8 m_nHops;
    u32 m_nXID;
    u16 m_nSecs;
    u16 m_nFlags;

    u32 m_nClientIPAddress;
    u32 m_nYourIPAddress;
    u32 m_nServerIPAddress;
    u32 m_nGatewayIPAddress;

    u8 m_aClientHWAddress[16];
    u8 m_aServerName[64];
    u8 m_aFile[128];

    u32 m_nMagic;

    u8 m_aOptions[308];
} PACKED;

struct DHCPOptionsWriter {
    u8* m_pBuffer;
    size_t m_nBufferSize;
    size_t m_nOffset;
};

template <size_t size>
struct DHCPOption {
    u8 m_nType;
    u8 m_nLength;
    u8 m_aValue[size];
};

struct DHCPClientState {
    // general
    u8 m_aMac[6];
    char m_szHostname[128];

    // sesion
    u32 m_nXID;
    u32 m_nDHCPServerIP;

    // contract
    u32 m_nOfferdIP;
    u32 m_nIPLeaseTimeS;
    u32 m_nSubnetMask;
    u32 m_nGateway;
};

typedef void(*DHCPSendPacketFN)(u32 nDstIp, u16 nDstPort, u16 nSrcPort, u8* pData, size_t nSize);

void DHCPOptionsWriterInit(DHCPOptionsWriter* pWriter, u8* pBuffer, size_t nBufferSize);
bool DHCPOptionsWriterAddOption(DHCPOptionsWriter* pWriter, u8 nType, u8* pData, size_t nLength);
bool DHCPOptionsWriterShutdown(DHCPOptionsWriter* pWriter);

u32 DHCPGenerateXID();
u8* DHCPGetOption(DHCPPacket* pPacket, u8 nType);
DHCPPacket DHCPCreateDiscoverPacket(const char* szHostName, u32 nXID, u8 pMac[6]);
DHCPPacket DHCPCreateRequestPacket(const char* szHostName, u32 nWantedIP, u32 nDHCPServerIP, u32 nXID, u8 pMac[6]);
DHCPPacket DHCPCreateLeaseExtendPacket(const char* szHostName, u32 nIpToExtend, u32 nXID, u8 pMac[6], u32 nDHCPServerIp);

/// @brief dhcp exports
extern "C" {
    DHCPClientState* DHCPClientInit(const char* szHostName, u8 pMac[6]);
    void DHCPClientNewSession(DHCPClientState* pClientState);
    void DHCPClientShutdown(DHCPClientState* pClientState);

    void DHCPClientStart(DHCPClientState* pClientState, DHCPSendPacketFN pSendPacket);
    int DHCPClientHandlePacket(DHCPClientState* pClientState, DHCPSendPacketFN pSendPacket, DHCPPacket* pPacket);

    void DHCPClientLeaseExtend(DHCPClientState* pClientState, DHCPSendPacketFN pSendPacket);
}

#endif // __DHCP_HPP__