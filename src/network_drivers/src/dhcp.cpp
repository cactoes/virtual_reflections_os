#define DRIVER_NAMING

#include "dhcp.hpp"
#include "std/random.hpp"
#include "virtual_reflections_driver.hpp"
#include "std/string.hpp"

void DHCPOptionsWriterInit(DHCPOptionsWriter* pWriter, uint8_t* pBuffer, size_t nBufferSize) {
    pWriter->m_pBuffer = pBuffer;
    pWriter->m_nBufferSize = nBufferSize;
    pWriter->m_nOffset = 0;
}

bool DHCPOptionsWriterAddOption(DHCPOptionsWriter* pWriter, uint8_t nType, uint8_t* pData, size_t nLength) {
    if (pWriter->m_nOffset + nLength + 2 > pWriter->m_nBufferSize)
        return false;

    pWriter->m_pBuffer[pWriter->m_nOffset++] = nType;
    pWriter->m_pBuffer[pWriter->m_nOffset++] = nLength;
    memcpy(&pWriter->m_pBuffer[pWriter->m_nOffset], pData, nLength);
    pWriter->m_nOffset += nLength;

    return true;
}

bool DHCPOptionsWriterShutdown(DHCPOptionsWriter* pWriter) {
    if (pWriter->m_nOffset >= pWriter->m_nBufferSize)
        return false;

    pWriter->m_pBuffer[pWriter->m_nOffset++] = DHCP_OPTION_END;
    return true;
}

uint32_t DHCPGenerateXID() {
    seed_random(KsTimeSinceBoot());
    return random_number(0, MAX_UINT32);
}

uint8_t* DHCPGetOption(DHCPPacket* pPacket, uint8_t nType) {
    uint8_t* pOptions = &pPacket->m_aOptions[0];

    while (true) {
        if (pOptions[0] == nType)
            return pOptions;

        if (pOptions[0] == DHCP_OPTION_END)
            return nullptr;

        pOptions += pOptions[1] + 2;
    }
}

DHCPPacket DHCPCreateDiscoverPacket(const char* szHostName, uint32_t nXID, uint8_t pMac[6]) {
    DHCPPacket dhcpPacket{};
    memzero(&dhcpPacket, sizeof(DHCPPacket));
    
    dhcpPacket.m_nOp = DHCP_OP_BOOTREQUEST;
    dhcpPacket.m_nHType = DHCP_HTYPE_ETHERNET;
    dhcpPacket.m_nHLen = DHCP_HLEN_ETHERNET_ADDRESS;
    dhcpPacket.m_nHops = 0;

    dhcpPacket.m_nXID = nXID;
    dhcpPacket.m_nSecs = bswap16(0);
    dhcpPacket.m_nFlags = bswap16(0x8000);

    dhcpPacket.m_nClientIPAddress = bswap32(0);
    dhcpPacket.m_nYourIPAddress = bswap32(0);
    dhcpPacket.m_nServerIPAddress = bswap32(0);
    dhcpPacket.m_nGatewayIPAddress = bswap32(0);

    for (size_t i = 0; i < 6; i++)
        dhcpPacket.m_aClientHWAddress[i] = pMac[i];

    dhcpPacket.m_nMagic = bswap32(DHCP_MAGIC);

    DHCPOptionsWriter dhcpOptionsWriter {};
    DHCPOptionsWriterInit(&dhcpOptionsWriter, &dhcpPacket.m_aOptions[0], sizeof(DHCPPacket::m_aOptions));

    uint8_t aDHCPMessageType[] { DHCP_MESSAGE_TYPE_DHCPDISCOVER };
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_DHCP_MESSAGE_TYPE, &aDHCPMessageType[0], sizeof(aDHCPMessageType));
    
    uint8_t aClientID[] { DHCP_HTYPE_ETHERNET, pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5] };
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_CLIENT_ID, &aClientID[0], sizeof(aClientID));
    
    size_t nHostNameLen = 0;
    for (const char* p = szHostName; *p; p++)
        nHostNameLen++;
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_HOSTNAME, (uint8_t*)szHostName, nHostNameLen);

    DHCPOptionsWriterShutdown(&dhcpOptionsWriter);

    return dhcpPacket;
}

DHCPPacket DHCPCreateRequestPacket(const char* szHostName, uint32_t nWantedIP, uint32_t nDHCPServerIP, uint32_t nXID, uint8_t pMac[6]) {
    DHCPPacket dhcpPacket{};
    memzero(&dhcpPacket, sizeof(DHCPPacket));
    
    dhcpPacket.m_nOp = DHCP_OP_BOOTREQUEST;
    dhcpPacket.m_nHType = DHCP_HTYPE_ETHERNET;
    dhcpPacket.m_nHLen = DHCP_HLEN_ETHERNET_ADDRESS;
    dhcpPacket.m_nHops = 0;

    dhcpPacket.m_nXID = nXID;
    dhcpPacket.m_nSecs = bswap16(0);
    dhcpPacket.m_nFlags = bswap16(0);

    dhcpPacket.m_nClientIPAddress = bswap32(0);
    dhcpPacket.m_nYourIPAddress = bswap32(0);
    dhcpPacket.m_nServerIPAddress = bswap32(nDHCPServerIP);
    dhcpPacket.m_nGatewayIPAddress = bswap32(0);

    for (size_t i = 0; i < 6; i++)
        dhcpPacket.m_aClientHWAddress[i] = pMac[i];

    dhcpPacket.m_nMagic = bswap32(DHCP_MAGIC);

    DHCPOptionsWriter dhcpOptionsWriter {};
    DHCPOptionsWriterInit(&dhcpOptionsWriter, &dhcpPacket.m_aOptions[0], sizeof(DHCPPacket::m_aOptions));

    uint8_t aDHCPMessageType[] { DHCP_MESSAGE_TYPE_DHCPREQUEST };
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_DHCP_MESSAGE_TYPE, &aDHCPMessageType[0], sizeof(aDHCPMessageType));
    
    uint32_t beWantedIP = bswap32(nWantedIP);
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_REQUESTED_IP_ADDR, (uint8_t*)&beWantedIP, 4);

    size_t nHostNameLen = strlen(szHostName);
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_HOSTNAME, (uint8_t*)szHostName, nHostNameLen);

    DHCPOptionsWriterShutdown(&dhcpOptionsWriter);

    return dhcpPacket;
}

DHCPPacket DHCPCreateLeaseExtendPacket(const char* szHostName, uint32_t nIpToExtend, uint32_t nXID, uint8_t pMac[6], uint32_t nDHCPServerIp) {
    DHCPPacket dhcpPacket{};
    memzero(&dhcpPacket, sizeof(DHCPPacket));
    
    dhcpPacket.m_nOp = DHCP_OP_BOOTREQUEST;
    dhcpPacket.m_nHType = DHCP_HTYPE_ETHERNET;
    dhcpPacket.m_nHLen = DHCP_HLEN_ETHERNET_ADDRESS;
    dhcpPacket.m_nHops = 0;

    dhcpPacket.m_nXID = nXID;
    dhcpPacket.m_nSecs = bswap16(0);
    dhcpPacket.m_nFlags = bswap16(0);

    dhcpPacket.m_nClientIPAddress = bswap32(nIpToExtend);
    dhcpPacket.m_nYourIPAddress = bswap32(0);
    dhcpPacket.m_nServerIPAddress = bswap32(0);
    dhcpPacket.m_nGatewayIPAddress = bswap32(0);

    for (size_t i = 0; i < 6; i++)
        dhcpPacket.m_aClientHWAddress[i] = pMac[i];

    dhcpPacket.m_nMagic = bswap32(DHCP_MAGIC);

    DHCPOptionsWriter dhcpOptionsWriter {};
    DHCPOptionsWriterInit(&dhcpOptionsWriter, &dhcpPacket.m_aOptions[0], sizeof(DHCPPacket::m_aOptions));

    uint8_t aDHCPMessageType[] { DHCP_MESSAGE_TYPE_DHCPREQUEST };
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_DHCP_MESSAGE_TYPE, &aDHCPMessageType[0], sizeof(aDHCPMessageType));

    uint32_t beExtendedIp = bswap32(nIpToExtend);
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_REQUESTED_IP_ADDR, (uint8_t*)&beExtendedIp, 4);

    uint32_t beDHCPServerIp = bswap32(nDHCPServerIp);
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_DHCP_SERVER_ID, (uint8_t*)&beDHCPServerIp, 4);

    size_t nHostNameLen = strlen(szHostName);
    DHCPOptionsWriterAddOption(&dhcpOptionsWriter, DHCP_OPTION_HOSTNAME, (uint8_t*)szHostName, nHostNameLen);

    DHCPOptionsWriterShutdown(&dhcpOptionsWriter);

    return dhcpPacket;
}

DHCPClientState* DHCPClientInit(const char* szHostName, uint8_t pMac[6]) {
    DHCPClientState* pClientState = (DHCPClientState*)malloc(sizeof(DHCPClientState));
    memzero(pClientState, sizeof(DHCPClientState));

    size_t nHostNameLen = strlen(szHostName);
    if (nHostNameLen >= 127)
        nHostNameLen = 127;

    memcpy(pClientState->m_szHostname, szHostName, nHostNameLen);
    memcpy(pClientState->m_aMac, pMac, 6);

    DHCPClientNewSession(pClientState);

    return pClientState;
}

void DHCPClientNewSession(DHCPClientState* pClientState) {
    pClientState->m_nXID = DHCPGenerateXID();
    pClientState->m_nOfferdIP = 0;
    pClientState->m_nDHCPServerIP = 0;
    pClientState->m_nIPLeaseTimeS = 0;
}

void DHCPClientShutdown(DHCPClientState* pClientState) {
    memzero(pClientState, sizeof(DHCPClientState));
    free(pClientState);
}

void DHCPClientStart(DHCPClientState* pClientState, DHCPSendPacketFN pSendPacket) {
    DHCPPacket dhcpPacket = DHCPCreateDiscoverPacket(pClientState->m_szHostname, pClientState->m_nXID, pClientState->m_aMac);
    pSendPacket(TO_IP(255, 255, 255, 255), DHCP_PORT_SERVER, DHCP_PORT_CLIENT, (uint8_t*)&dhcpPacket, sizeof(DHCPPacket));
}

uint32_t DHCPFieldToNumber(uint8_t* pField, size_t nSize) {
    uint32_t num = 0;
    for (size_t i = 0; i < nSize; i++) {
        num <<= 8;
        num += pField[i];
    }
    return num;
}

int DHCPClientHandlePacket(DHCPClientState* pClientState, DHCPSendPacketFN pSendPacket, DHCPPacket* pPacket) {
    if (pPacket->m_nXID != pClientState->m_nXID)
        return DHCP_CLIENT_RECIEVE_ERR;

    DHCPOption<1>* pMessageTypeOption = (DHCPOption<1>*)DHCPGetOption(pPacket, DHCP_OPTION_DHCP_MESSAGE_TYPE);
    if (!pMessageTypeOption)
        return DHCP_CLIENT_RECIEVE_ERR;

    DHCPOption<4>* pSubnetMaskOption = (DHCPOption<4>*)DHCPGetOption(pPacket, DHCP_OPTION_SUBNET_MASK);
    DHCPOption<4>* pRouterOption = (DHCPOption<4>*)DHCPGetOption(pPacket, DHCP_OPTION_ROUTER);
    DHCPOption<4>* pDHCPServerIDOption = (DHCPOption<4>*)DHCPGetOption(pPacket, DHCP_OPTION_DHCP_SERVER_ID);
    DHCPOption<4>* pIPLeaseTime = (DHCPOption<4>*)DHCPGetOption(pPacket, DHCP_OPTION_IP_LEASE_TIME);

    switch (pMessageTypeOption->m_aValue[0]) {
        case DHCP_MESSAGE_TYPE_DHCPACK: {
            if (pClientState->m_nOfferdIP != bswap32(pPacket->m_nYourIPAddress) ||
                !pSubnetMaskOption || !pRouterOption)
                return DHCP_CLIENT_RECIEVE_ERR;

            pClientState->m_nSubnetMask = TO_IP(pSubnetMaskOption->m_aValue[0], pSubnetMaskOption->m_aValue[1], pSubnetMaskOption->m_aValue[2], pSubnetMaskOption->m_aValue[3]);
            pClientState->m_nGateway = TO_IP(pRouterOption->m_aValue[0], pRouterOption->m_aValue[1], pRouterOption->m_aValue[2], pRouterOption->m_aValue[3]);

            return DHCP_CLIENT_RECIEVE_ACK;
        }
        case DHCP_MESSAGE_TYPE_DHCPOFFER: {
            if (!pDHCPServerIDOption || !pIPLeaseTime)
                return DHCP_CLIENT_RECIEVE_ERR;

            pClientState->m_nOfferdIP = bswap32(pPacket->m_nYourIPAddress);
            pClientState->m_nDHCPServerIP = TO_IP(pDHCPServerIDOption->m_aValue[0], pDHCPServerIDOption->m_aValue[1], pDHCPServerIDOption->m_aValue[2], pDHCPServerIDOption->m_aValue[3]);
            pClientState->m_nIPLeaseTimeS = DHCPFieldToNumber(&pIPLeaseTime->m_aValue[0], 4);

            DHCPPacket dhcpPacket = DHCPCreateRequestPacket(pClientState->m_szHostname, pClientState->m_nOfferdIP, pClientState->m_nDHCPServerIP, pClientState->m_nXID, pClientState->m_aMac);
            pSendPacket(pClientState->m_nDHCPServerIP, DHCP_PORT_SERVER, DHCP_PORT_CLIENT, (uint8_t*)&dhcpPacket, sizeof(DHCPPacket));
            return DHCP_CLIENT_RECIEVE_REQ;
        }
        default:
            return DHCP_CLIENT_RECIEVE_OTH;
    }

    return DHCP_CLIENT_RECIEVE_ERR;
}

void DHCPClientLeaseExtend(DHCPClientState* pClientState, DHCPSendPacketFN pSendPacket) {
    DHCPPacket dhcpPacket = DHCPCreateLeaseExtendPacket(pClientState->m_szHostname, pClientState->m_nOfferdIP, pClientState->m_nXID, pClientState->m_aMac, pClientState->m_nDHCPServerIP);
    pSendPacket(pClientState->m_nDHCPServerIP, DHCP_PORT_SERVER, DHCP_PORT_CLIENT, (uint8_t*)&dhcpPacket, sizeof(DHCPPacket));
}