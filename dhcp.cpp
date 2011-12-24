// DHCP look-up functions based on the udp client
// http://www.ietf.org/rfc/rfc2131.txt
//
// Author: Andrew Lindsay
// Rewritten and optimized by Jean-Claude Wippler, http://jeelabs.org/
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include "EtherCard.h"
#include "net.h"

#define gPB ether.buffer

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTRESPONSE 2

// DHCP States for access in applications
enum {
    DHCP_STATE_INIT,
    DHCP_STATE_DISCOVER,
    DHCP_STATE_OFFER,
    DHCP_STATE_REQUEST,
    DHCP_STATE_ACK,
    DHCP_STATE_OK,
    DHCP_STATE_RENEW,
};

// size 236
typedef struct {
    byte op, htype, hlen, hops;
    uint32_t xid;
    word secs, flags;
    byte ciaddr[4], yiaddr[4], siaddr[4], giaddr[4];
    byte chaddr[16], sname[64], file[128];
    // options
} DHCPdata;

// src port high byte must be a a0 or higher:
#define DHCPCLIENT_SRC_PORT_H 0xe0 
#define DHCP_SRC_PORT 67
#define DHCP_DEST_PORT 68

static byte dhcpState;
static char hostname[] = "Arduino-00";
static uint32_t currentXid;
static uint32_t leaseStart;
static uint32_t leaseTime;
static byte* bufPtr;

static const byte allOnes[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void addToBuf (byte b) {
    *bufPtr++ = b;
}

static void addBytes (byte len, const byte* data) {
    while (len-- > 0)
        addToBuf(*data++);
}

static byte dhcp_ready () {
    if (dhcpState == DHCP_STATE_OK && millis() >= leaseStart + leaseTime)
        dhcpState = DHCP_STATE_RENEW;
    return dhcpState == DHCP_STATE_OK;
}

// Main DHCP sending function, either DHCP_STATE_DISCOVER or DHCP_STATE_REQUEST
static void dhcp_send (byte request) {
    if (!EtherCard::isLinkUp())
        return;
    dhcpState = request;
    memset(gPB, 0, UDP_DATA_P + sizeof( DHCPdata ));
    EtherCard::udpPrepare(DHCP_DEST_PORT, EtherCard::myip, DHCP_SRC_PORT);
    EtherCard::copyMac(gPB + ETH_SRC_MAC, EtherCard::mymac);
    EtherCard::copyMac(gPB + ETH_DST_MAC, allOnes);
    gPB[IP_TOTLEN_L_P]=0x82;
    gPB[IP_PROTO_P]=IP_PROTO_UDP_V;
    EtherCard::copyIp(gPB + IP_DST_P, allOnes);
    
    // Build DHCP Packet from buf[UDP_DATA_P]
    DHCPdata *dhcpPtr = (DHCPdata*) (gPB + UDP_DATA_P);
    dhcpPtr->op = DHCP_BOOTREQUEST;
    dhcpPtr->htype = 1;
    dhcpPtr->hlen = 6;
    dhcpPtr->xid = currentXid;
    EtherCard::copyMac(dhcpPtr->chaddr, EtherCard::mymac);
    
    // options defined as option, length, value
    bufPtr = gPB + UDP_DATA_P + sizeof( DHCPdata );
    // DHCP magic cookie, followed by message type
    static byte cookie[] = { 99, 130, 83, 99, 53, 1 };
    addBytes(sizeof cookie, cookie);
    // addToBuf(53);  // DHCP_STATE_DISCOVER, DHCP_STATE_REQUEST
    // addToBuf(1);   // Length 
    addToBuf(dhcpState);
    
    // Client Identifier Option, this is the client mac address
    addToBuf(61);     // Client identifier
    addToBuf(7);      // Length 
    addToBuf(0x01);   // Ethernet
    addBytes(6, EtherCard::mymac);
    
    addToBuf(12);     // Host name Option
    addToBuf(10);
    addBytes(10, (byte*) hostname);
    
    if( dhcpState == DHCP_STATE_REQUEST) {
        addToBuf(50); // Request IP address
        addToBuf(4);
        addBytes(4, EtherCard::myip);

        // Request using server ip address
        addToBuf(54); // Server IP address
        addToBuf(4);
        addBytes(4, EtherCard::dhcpip);
    }
    
    // Additional info in parameter list - minimal list for what we need
    static byte tail[] = { 55, 3, 1, 3, 6, 255 };
    addBytes(sizeof tail, tail);
    // addToBuf(55);     // Parameter request list
    // addToBuf(3);      // Length 
    // addToBuf(1);      // Subnet mask
    // addToBuf(3);      // Route/Gateway
    // addToBuf(6);      // DNS Server
    // addToBuf(255);    // end option

    // packet size will be under 300 bytes
    EtherCard::udpTransmit((bufPtr - gPB) - UDP_DATA_P);
}

static void have_dhcpoffer (word len) {
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata*) (gPB + UDP_DATA_P);
    // Offered IP address is in yiaddr
    EtherCard::copyIp(EtherCard::myip, dhcpPtr->yiaddr);
    // Scan through variable length option list identifying options we want
    byte *ptr = (byte*) (dhcpPtr + 1) + 4;
    do {
        byte option = *ptr++;
        byte optionLen = *ptr++;
        switch (option) {
            case 1:  EtherCard::copyIp(EtherCard::mymask, ptr);
                     break;
            case 3:  EtherCard::copyIp(EtherCard::gwip, ptr);
                     break;
            case 6:  EtherCard::copyIp(EtherCard::dnsip, ptr);
                     break;
            case 51: leaseTime = 0;
                     for (byte i = 0; i<4; i++)
                         leaseTime = (leaseTime + ptr[i]) << 8;
                     leaseTime *= 1000;      // milliseconds
                     break;
            case 54: EtherCard::copyIp(EtherCard::dhcpip, ptr);
                     break;
        }
        ptr += optionLen;
    } while (ptr < gPB + len);
    dhcp_send(DHCP_STATE_REQUEST);
}

static void have_dhcpack (word /*len*/) {
    dhcpState = DHCP_STATE_OK;
    leaseStart = millis();
}

static void check_for_dhcp_answer (word len) {
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata*) (gPB + UDP_DATA_P);
    if (len >= 70 && gPB[UDP_SRC_PORT_L_P] == DHCP_SRC_PORT &&
            dhcpPtr->op == DHCP_BOOTRESPONSE && dhcpPtr->xid == currentXid ) {
        int optionIndex = UDP_DATA_P + sizeof( DHCPdata ) + 4;
        if (gPB[optionIndex] == 53) {
            switch( gPB[optionIndex+2] ) {
                case DHCP_STATE_OFFER: have_dhcpoffer(len); break;
                case DHCP_STATE_OK:    have_dhcpack(len); break;
            }
        }
    }
}

// use during setup, as this discards all incoming requests until it returns
bool EtherCard::dhcpSetup () {
  currentXid = millis();
  // Set a unique hostname, use Arduino-?? with last octet of mac address
  hostname[8] = 'A' + (mymac[5] >> 4);
  hostname[9] = 'A' + (mymac[5] & 0x0F);
  myip[0] = 0; // force invalid IP address

  // Enable reception of broadcast packets as some DHCP servers
  // use this to send responses
  enableBroadcast();
  for (byte i = 0; i < 3; ++i) {
    dhcpState = DHCP_STATE_INIT;
    word start = millis();
    while (myip[0] == 0 && (word) (millis() - start) < 10000) {
      if (!isLinkUp())
        continue;
      word len = packetReceive();
      if (dhcpState != DHCP_STATE_INIT && (len == 0 || packetLoop(len) > 0))
        continue;
      
      switch (dhcpState) {
          case DHCP_STATE_INIT:
          case DHCP_STATE_RENEW:
              dhcp_send(DHCP_STATE_DISCOVER);
              break;
          case DHCP_STATE_DISCOVER:
          case DHCP_STATE_REQUEST:
              check_for_dhcp_answer(len);
              break;
          case DHCP_STATE_OK:
              ; //TODO wait for lease expiration
      }
    
      if (myip[0] != 0) {
        if (gwip[0] != 0)
          setGwIp(gwip);
        disableBroadcast();
        return true;
      }
    }
  }
  disableBroadcast();
  return false;
}

bool EtherCard::dhcpExpired () {
  return !dhcp_ready() && dhcpState == DHCP_STATE_RENEW;
}
