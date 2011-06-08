// DHCP look-up functions based on the udp client
// http://www.ietf.org/rfc/rfc2131.txt
//
// Author: Andrew Lindsay
// Rewritten and optimized by Jean-Claude Wippler, http://jeelabs.org/
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include <WProgram.h>
#include "ip_config.h"
#include "net.h"
#include "dhcp.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTRESPONSE 2

// DHCP States for access in applications
#define DHCP_STATE_INIT 0
#define DHCP_STATE_DISCOVER 1
#define DHCP_STATE_OFFER 2
#define DHCP_STATE_REQUEST 3
#define DHCP_STATE_ACK 4
#define DHCP_STATE_OK 5
#define DHCP_STATE_RENEW 6

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

// Pointers to values we have set or need to set
static byte *macaddr;
static DHCPinfo *infoPtr;

static byte dhcpState;
static byte dhcptid_l; // a counter for transaction ID
static char hostname[] = "Arduino-00";
static uint32_t currentXid;
static uint32_t leaseStart;
static uint32_t leaseTime;
static byte* bufPtr;

static void addToBuf (byte b) {
    *bufPtr++ = b;
}

static void addBytes (byte len, const byte* data) {
    while (len-- > 0)
        addToBuf(*data++);
}

static byte dhcp_ready(void) {
    if (dhcpState == DHCP_STATE_OK && leaseStart + leaseTime <= millis())
        dhcpState = DHCP_STATE_RENEW;
    return dhcpState == DHCP_STATE_OK;
}

// Main DHCP sending function, either DHCP_STATE_DISCOVER or DHCP_STATE_REQUEST
static void dhcp_send (byte *buf, byte request) {
    if (!enc28j60linkup())
        return;
    dhcpState = request == 0 ? DHCP_STATE_DISCOVER : DHCP_STATE_REQUEST;
    dhcptid_l++; // increment for next request, finally wrap
    memset(buf, 0, UDP_DATA_P + sizeof( DHCPdata ));
    send_udp_prepare(buf,(DHCPCLIENT_SRC_PORT_H<<8)|dhcptid_l,infoPtr->myip,
                                                        DHCP_DEST_PORT);
    memset(buf + ETH_DST_MAC, 0xFF, 6);
    buf[IP_TOTLEN_L_P]=0x82;
    buf[IP_PROTO_P]=IP_PROTO_UDP_V;
    memset(buf + IP_DST_P, 0xFF, 4);
    buf[UDP_DST_PORT_L_P]=DHCP_SRC_PORT; 
    buf[UDP_SRC_PORT_H_P]=0;
    buf[UDP_SRC_PORT_L_P]=DHCP_DEST_PORT;
    
    // Build DHCP Packet from buf[UDP_DATA_P]
    DHCPdata *dhcpPtr = (DHCPdata*) (buf + UDP_DATA_P);
    dhcpPtr->op = DHCP_BOOTREQUEST;
    dhcpPtr->htype = 1;
    dhcpPtr->hlen = 6;
    dhcpPtr->xid = currentXid;
    copy6(dhcpPtr->chaddr, macaddr);
    
    // options defined as option, length, value
    bufPtr = buf + UDP_DATA_P + sizeof( DHCPdata );
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
    addBytes(6, macaddr);
    
    addToBuf(12);     // Host name Option
    addToBuf(10);
    addBytes(10, (byte*) hostname);
    
    if( dhcpState == DHCP_STATE_REQUEST) {
        addToBuf(50); // Request IP address
        addToBuf(4);
        addBytes(4, infoPtr->myip);
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
    send_udp_transmit(buf, (bufPtr - buf) - UDP_DATA_P);
}

static void have_dhcpoffer (byte *buf, word len) {
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata*) (buf + UDP_DATA_P);
    // Offered IP address is in yiaddr
    copy4(infoPtr->myip, dhcpPtr->yiaddr);
    // Scan through variable length option list identifying options we want
    byte *ptr = (byte*) (dhcpPtr + 1) + 4;
    do {
        byte option = *ptr++;
        byte optionLen = *ptr++;
        switch (option) {
            case 1:  copy4(infoPtr->mymask, ptr);
                     break;
            case 3:  copy4(infoPtr->gwip, ptr);
                     break;
            case 6:  copy4(infoPtr->dnsip, ptr);
                     break;
            case 51: leaseTime = 0;
                     for (byte i = 0; i<4; i++)
                         leaseTime = (leaseTime + ptr[i]) << 8;
                     leaseTime *= 1000;      // milliseconds
                     break;
        }
        ptr += optionLen;
    } while (ptr < buf + len);
    dhcp_send(buf, 1);
}

static void have_dhcpack (byte *buf, word len) {
    dhcpState = DHCP_STATE_OK;
    leaseStart = millis();
}

static void check_for_dhcp_answer (byte *buf, word len) {
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata*) (buf + UDP_DATA_P);
    if (len >= 70 && buf[UDP_SRC_PORT_L_P] == DHCP_SRC_PORT &&
            dhcpPtr->op == DHCP_BOOTRESPONSE && dhcpPtr->xid == currentXid ) {
        int optionIndex = UDP_DATA_P + sizeof( DHCPdata ) + 4;
        if (buf[optionIndex] == 53) {
            switch( buf[optionIndex+2] ) {
                case DHCP_STATE_OFFER: have_dhcpoffer(buf, len); break;
                case DHCP_STATE_OK:    have_dhcpack(buf, len); break;
            }
        }
    }
}

void DHCP::dhcpInit (byte* macaddrin, DHCPinfo& di) {
    macaddr = macaddrin;
    infoPtr = &di;
    currentXid = millis();
    memset(infoPtr, 0, sizeof *infoPtr);
    // Set a unique hostname, use Arduino-?? with last octet of mac address
    hostname[8] = 'A' + (macaddr[6] >> 4);
    hostname[9] = 'A' + (macaddr[6] & 0x0F);
}

byte DHCP::dhcpCheck (byte *buf, word len) {
    if (macaddr == 0)
        return 0;
    switch (dhcpState) {
        case DHCP_STATE_INIT:
        case DHCP_STATE_RENEW:
            dhcp_send(buf, 0);
            break;
        case DHCP_STATE_DISCOVER:
        case DHCP_STATE_REQUEST:
            check_for_dhcp_answer(buf, len);
            //TODO resend request on timeout
            break;
        case DHCP_STATE_OK:
            ; //TODO wait for lease expiration
    }
    return infoPtr->myip[0] != 0;
}
