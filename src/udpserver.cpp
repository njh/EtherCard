// Simple UDP listening server
//
// Author: Brian Lee
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include "EtherCard.h"
#include "EtherUtil.h"
#include "net.h"

#define UDPSERVER_MAXLISTENERS 8    //the maximum number of port listeners.

typedef struct {
    UdpServerCallback callback;
    uint16_t port;
    bool listening;
} UdpServerListener;

UdpServerListener listeners[UDPSERVER_MAXLISTENERS];
byte numListeners = 0;

void EtherCard::udpServerListenOnPort(UdpServerCallback callback, uint16_t port) {
    if(numListeners < UDPSERVER_MAXLISTENERS)
    {
        listeners[numListeners] = (UdpServerListener) {
            callback, port, true
        };
        numListeners++;
    }
}

static void udp_listen_on_port(const uint16_t port, const bool listen)
{
    for (UdpServerListener *iter = listeners, *last = listeners + numListeners;
            iter != last; ++iter)
    {
        UdpServerListener &l = *iter;
        if(l.port == port)
        {
            l.listening = listen;
            break;
        }
    }
}

void EtherCard::udpServerPauseListenOnPort(uint16_t port) {
    udp_listen_on_port(port, false);
}

void EtherCard::udpServerResumeListenOnPort(uint16_t port) {
    udp_listen_on_port(port, true);
}

bool EtherCard::udpServerListening() {
    return numListeners > 0;
}

bool EtherCard::udpServerHasProcessedPacket(const IpHeader &iph, const uint8_t *iter, const uint8_t *last) {
    bool packetProcessed = false;
    UdpHeader &udph = udp_header();
    const uint16_t dport = ntohs(udph.dport);
    for (UdpServerListener *iter = listeners, *last = listeners + numListeners;
            iter != last; ++iter)
    {
        UdpServerListener &l = *iter;
        if (l.listening && l.port == dport)
        {
            const uint16_t datalen = udph.length - sizeof(UdpHeader);
            l.callback(
                l.port,
                (uint8_t *)iph.spaddr, // TODO: change definition of UdpServerCallback to const uint8_t *
                ntohs(udph.sport),
                (const char *)udp_payload(),
                datalen);
            packetProcessed = true;
        }
    }
    return packetProcessed;
}
