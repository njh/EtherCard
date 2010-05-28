// Microchip ENC28J60 Ethernet Interface Driver
// Author: Pascal Stang 
// Modified by: Guido Socher
// Copyright: GPL V2
// 
// This driver provides initialization and transmit/receive
// functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
// chip, using an SPI interface to the host processor.
//
// Mods bij jcw, 2010-05-20

#ifndef ENC28J60_H
#define ENC28J60_H

#include <inttypes.h>

// legacy
extern uint8_t enc28j60Init(uint8_t* macaddr);
extern void enc28j60PacketSend(uint16_t len, uint8_t* packet);
extern uint8_t enc28j60hasRxPkt();
extern uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet);
extern uint8_t enc28j60linkup();

class ENC28J60 {
public:
    void spiInit();
    
    uint8_t initialize(uint8_t* macaddr)
        { return enc28j60Init(macaddr); }
    
    void packetSend(uint8_t* packet, uint16_t len)
        { enc28j60PacketSend(len, packet); }
    
    uint16_t packetReceive(uint8_t* packet, uint16_t maxlen)
        { return enc28j60PacketReceive(maxlen, packet); }
};

#endif
