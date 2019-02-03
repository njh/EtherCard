// Shortcuts to different protocols headers/payload
// Hence: GPL V2

#ifndef ETHERUTIL_H
#define ETHERUTIL_H

#include "enc28j60.h"
#include "net.h"

#define gPB ether.buffer

inline EthHeader &ethernet_header()
{
    uint8_t *iter = gPB; // avoid strict aliasing warning
    return *(EthHeader *)iter;
}

inline uint8_t *ethernet_payload()
{
    return (uint8_t *)&ethernet_header() + sizeof(EthHeader);
}

inline ArpHeader &arp_header()
{
    return *(ArpHeader *)ethernet_payload();
}

inline IpHeader &ip_header()
{
    return *(IpHeader *)ethernet_payload();
}

inline uint8_t *ip_payload()
{
    return (uint8_t *)&ip_header() + sizeof(IpHeader);
}

inline uint8_t *tcp_header()
{
    return (uint8_t *)ip_payload();
}

inline UdpHeader &udp_header()
{
    return *(UdpHeader *)ip_payload();
}

inline uint8_t *udp_payload()
{
    return (uint8_t *)&udp_header() + sizeof(UdpHeader);
}

#endif /* ETHERUTIL_H */
