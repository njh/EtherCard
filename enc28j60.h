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
// 2010-05-20 <jc@wippler.nl>

#ifndef ENC28J60_H
#define ENC28J60_H

class ENC28J60 {
public:
  static uint8_t buffer[];
  static uint16_t bufferSize;
  
  static uint8_t* tcpOffset () { return buffer + 0x36; }

  static void initSPI ();
  static uint8_t initialize (const uint16_t size, const uint8_t* macaddr,
                             uint8_t csPin =8);
  static bool isLinkUp ();
  
  static void packetSend (uint16_t len);
  static uint16_t packetReceive ();
  
  static void copyout (uint8_t page, const uint8_t* data);
  static void copyin (uint8_t page, uint8_t* data);
  static uint8_t peekin (uint8_t page, uint8_t off);

  static void powerDown();  // contrib by Alex M.
  static void powerUp();    // contrib by Alex M.
  
  static void enableBroadcast();
  static void disableBroadcast();

  static uint8_t doBIST(uint8_t csPin =8);
};

typedef ENC28J60 Ethernet;

#endif
