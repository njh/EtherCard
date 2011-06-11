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

class ENC28J60 {
public:
  static void spiInit ();
  static uint8_t initialize (uint8_t* macaddr);
  static bool isLinkUp ();
  static void packetSend (const uint8_t* packet, uint16_t len);
  static uint16_t packetReceive (uint8_t* packet, uint16_t maxlen);
  static void copyout (uint8_t page, const uint8_t* data);
  static void copyin (uint8_t page, uint8_t* data);
};

#endif
