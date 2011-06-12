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

extern uint8_t gPacketBuffer[];
extern uint16_t gPacketBufferLength;

class ENC28J60 {
public:
  ENC28J60 (const uint16_t bufferSize) { gPacketBufferLength = bufferSize; }
  static uint8_t* tcpOffset () { return gPacketBuffer + 0x36; }

  static void spiInit ();
  static uint8_t initialize (uint8_t* macaddr);
  static bool isLinkUp ();
  static void packetSend (uint16_t len);
  static uint16_t packetReceive ();
  static void copyout (uint8_t page, const uint8_t* data);
  static void copyin (uint8_t page, uint8_t* data);
  
  uint16_t bufferLimit;
};

typedef ENC28J60 Ethernet;

#endif
