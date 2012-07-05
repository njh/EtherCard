// DHCP look-up functions based on the udp client
// http://www.ietf.org/rfc/rfc2131.txt
//
// Author: Andrew Lindsay
// Rewritten and optimized by Jean-Claude Wippler, http://jeelabs.org/
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html
//
// $Revision$
//

#if 0
# define __STDC_LIMIT_MACROS
# include <stdint.h>     // Used only for UINT32_MAX
#else
# define UINT32_MAX (4294967295U)
#endif
#include "EtherCard.h"
#include "net.h"

#define gPB ether.buffer

#define DHCP_BOOTREQUEST 1
#define DHCP_BOOTRESPONSE 2

// DHCP FSM States
enum {
    DHCP_STATE_BAD,
    DHCP_STATE_INIT,
    DHCP_STATE_SELECT,
    DHCP_STATE_REQUEST,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEW,
    DHCP_STATE_REBIND
};

// DHCP Messages
// These values are fixed by the protocol
enum {
    DHCP_MSG_NONE,
    DHCP_MSG_DISCOVER,
    DHCP_MSG_OFFER,
    DHCP_MSG_REQUEST,
    DHCP_MSG_DECLINE,
    DHCP_MSG_ACK,
    DHCP_MSG_NACK,
    DHCP_MSG_RELEASE,
    DHCP_MSG_INFORM
};

// DHCP message header, size 236 bytes
typedef struct {
    byte op;        // Message type
    byte htype;     // Hardware type
    byte hlen;      // Hardware address length
    byte hops;      // Relay count
    uint32_t xid;   // Transaction ID
    word secs;      // Seconds elapsed
    word flags;     // Broadcast flag
    byte ciaddr[4]; // Client IP Address
    byte yiaddr[4]; // Your IP Address
    byte siaddr[4]; // Server IP Address
    byte giaddr[4]; // Gateway IP Address
    byte chaddr[16];// Client Hardware Address
    byte sname[64]; // Server host name
    byte file[128]; // BOOTP legacy
    byte magic[4];  // Magic Cookie
                    // options
} DHCPdata;

// src port high byte must be a a0 or higher:
#define DHCPCLIENT_SRC_PORT_H 0xe0
#define DHCP_SRC_PORT 67
#define DHCP_DEST_PORT 68
#define DHCP_WAIT 20000 // msec to wait for DHCP address

static byte dhcpState;  // Current DHCP FSM state
static char hostname[16] = "Arduino-00";
static uint32_t currentXid;
static uint32_t leaseStart;
static uint32_t leaseTime;
static byte* bufPtr;

static const byte allZeros[] = { 0, 0, 0, 0, 0, 0 };

static const byte allOnes[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void addToBuf (byte b) {
    *bufPtr++ = b;
}

static void addBytes (byte len, const byte* data) {
    while (len-- > 0)
        addToBuf(*data++);
}

// Return the DHCP lease start time
uint32_t EtherCard::dhcpStartTime () {
    return leaseStart;
}

// Return the DHCP lease duration
uint32_t EtherCard::dhcpLeaseTime () {
    return leaseTime;
}

// Return the DHCP FSM state
byte EtherCard::dhcpFSM () {
    return dhcpState;
}

// Does the DHCP FSM have a valid lease?
bool EtherCard::dhcpValid () {
    return dhcpState == DHCP_STATE_BOUND ||
           dhcpState == DHCP_STATE_RENEW ||
           dhcpState == DHCP_STATE_REBIND;
}

// Build and transmit a DHCP message.
// The argument is either DHCP_MSG_DISCOVER or DHCP_MSG_REQUEST
static void dhcp_send (byte request, byte flg) {
    // How should this error be propagated ???
    if (!EtherCard::isLinkUp())
        return;

    // Start with a clean buffer
    memset(gPB, 0, UDP_DATA_P + sizeof(DHCPdata));
    EtherCard::udpPrepare(DHCP_DEST_PORT, EtherCard::myip, DHCP_SRC_PORT);
    EtherCard::copyMac(gPB + ETH_SRC_MAC, EtherCard::mymac);
    EtherCard::copyMac(gPB + ETH_DST_MAC, allOnes);
    gPB[IP_TOTLEN_L_P] = 0x82;
    gPB[IP_PROTO_P] = IP_PROTO_UDP_V;
    // If we don't know where we're going, go everywhere
    if (flg)
        EtherCard::copyIp(gPB + IP_DST_P, allOnes);
    else
        EtherCard::copyIp(gPB + IP_DST_P, EtherCard::dhcpip);

    // Build a DHCP Packet from buf[UDP_DATA_P]
    DHCPdata *dhcpPtr = (DHCPdata *) (gPB + UDP_DATA_P);
    dhcpPtr->op = DHCP_BOOTREQUEST;
    dhcpPtr->htype = 1;
    dhcpPtr->hlen = 6;
    dhcpPtr->hops = 0;
    dhcpPtr->xid = currentXid;
    dhcpPtr->secs = 0;
    dhcpPtr->flags = flg ? 0x80 : 0; // Set broadcast flag
    //dhcpPtr->ciaddr
    //dhcpPtr->yiaddr
    //dhcpPtr->siaddr
    //dhcpPtr->giaddr
    EtherCard::copyMac(dhcpPtr->chaddr, EtherCard::mymac);
    //dhcpPtr->sname
    //dhcpPtr->file

    // Write the cookie 0x63,0x82,0x53,0x63 into the end of the header
    static byte cookie[] = { 99, 130, 83, 99 };
    bufPtr = gPB + UDP_DATA_P + sizeof(DHCPdata) - sizeof(cookie);
    addBytes(sizeof(cookie), cookie);
    // Options are defined as option, length, value
    addToBuf(53);     // DHCP message type
    addToBuf(1);      // Length
    addToBuf(request);// DHCP_MSG_DISCOVER, DHCP_MSG_REQUEST

    // Client Identifier Option, this is the client mac address
    addToBuf(61);     // Client identifier
    addToBuf(7);      // Length
    addToBuf(0x01);   // Ethernet
    addBytes(6, EtherCard::mymac);

    addToBuf(12);     // Host name Option
    byte len = (byte) strlen(hostname);
    addToBuf(len);
    addBytes(len, (byte *) hostname);

    if (request == DHCP_MSG_REQUEST) {
        addToBuf(50); // Request IP address
        addToBuf(4);
        addBytes(4, EtherCard::myip);

        addToBuf(54); // Server IP address
        addToBuf(4);
        addBytes(4, EtherCard::dhcpip);
    }

    // Additional info in parameter list - minimal list for what we need
    // addToBuf(55);     // Parameter request list
    // addToBuf(3);      // Length
    // addToBuf(1);      // Subnet mask
    // addToBuf(3);      // Route/Gateway
    // addToBuf(6);      // DNS Server
    // addToBuf(255);    // end option
    static byte tail[] = { 55, 3, 1, 3, 6, 255 };
    addBytes(sizeof(tail), tail);

    // The packet size will be under 300 bytes
    EtherCard::udpTransmit((bufPtr - gPB) - UDP_DATA_P);

    return;
}

// Parse a DHCP offer message.
static void parse_dhcpoffer (word len) {
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata *) (gPB + UDP_DATA_P);
    // Offered IP address is in yiaddr
    EtherCard::copyIp(EtherCard::myip, dhcpPtr->yiaddr);
    // Scan through variable length option list identifying options we want
    byte *ptr = (byte *) (dhcpPtr + 1);
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
                     leaseTime = (((uint32_t) ptr[0]) << 24) +
                                 (((uint32_t) ptr[1]) << 16) +
                                 (((uint32_t) ptr[2]) << 8) + ptr[3];
                     leaseTime *= 1000;      // milliseconds
                     break;
            case 54: EtherCard::copyIp(EtherCard::dhcpip, ptr);
                     break;
        }
        ptr += optionLen;
    } while (ptr < (gPB + len));

    return;
}

// Check an incoming DHCP message and decide what to do with it.
//
// Returns:     The next FSM state  Success
//              DHCP_STATE_BAD      Otherwise
//
static byte check_for_dhcp_answer (word len) {
    byte rc = DHCP_STATE_BAD;
    // Map struct onto payload
    DHCPdata *dhcpPtr = (DHCPdata *) (gPB + UDP_DATA_P);
    // len for the minimal UDP packet is:
    // Ethernet header - preamble/SFD = 14
    // IP header = 20
    // UDP header = 8
    // The trailing Ethernet CRC is dropped in software, so
    // the minimal UDP packet, one with no data, has len 42 
    // Coincidence?  I think _not_.
    if (len > 42 &&
                gPB[UDP_SRC_PORT_L_P] == DHCP_SRC_PORT &&
                dhcpPtr->op == DHCP_BOOTRESPONSE &&
                dhcpPtr->xid == currentXid) {
        // Scan for the Message Type Option
        byte *sp = (byte *) (gPB + UDP_DATA_P + sizeof(DHCPdata));
        byte *bp = sp;
        // Is it worth checking for pointer overrun ???
        uint16_t offset = (gPB[IP_TOTLEN_H_P] << 8) + gPB[IP_TOTLEN_L_P];
        offset -= (20 + 8 + sizeof(DHCPdata));  // IP hdr 20, UDP hdr 8
        while (*bp != 0xff && (bp - sp) < offset) {
            // Get the DHCP message type
            if (*bp == 53) {
                switch(*(bp + 2)) {
                // DHCP_STATE_INIT sends a DHCP_MSG_DISCOVER
                // which returns a DHCP_MSG_OFFER
                case DHCP_MSG_OFFER:
                    // The offer message is parsed in the main FSM
                    rc = DHCP_STATE_SELECT;
                    break;
                // DHCP_STATE_SELECT sends a DHCP_MSG_REQUEST
                // which returns a DHCP_MSG_ACK
                // A DHCP_STATE_BOUND timeout sends a DHCP_MSG_REQUEST
                // which returns a DHCP_MSG_ACK
                // A DHCP_STATE_RENEW timeout sends a DHCP_MSG_REQUEST
                // which returns a DHCP_MSG_ACK
                // DHCP_STATE_REBIND sends a DHCP_MSG_REQUEST
                // which returns a DHCP_MSG_ACK
                case DHCP_MSG_ACK:
                    switch (dhcpState) {
                    case DHCP_STATE_REQUEST:
                        rc = DHCP_STATE_REQUEST;
                        break;
                    case DHCP_STATE_RENEW:
                        rc = DHCP_STATE_BOUND;
                        break;
                    case DHCP_STATE_REBIND:
                        rc = DHCP_STATE_BOUND;
                        break;
                    default:
                        break;
                    }
                    break;
                // This message forces a full re-initialisation
                case DHCP_MSG_NACK:
                    rc = DHCP_STATE_INIT;
                    break;
                default:
                    // What is to be done with unwanted messages ???
                    break;
                }
                // Once we've got the message type we're done
                break;
            }
            else {
                // Skip over the current option
                bp = bp + *(bp + 1) + 1;
            }
        }
    }

    return rc;
}

// This handles both the initialisation of DHCP, and subsequent
// renegotiations.  Lacking a dedicated timer, EtherCard::dhcpLease()
// must be called on a regular basis to keep things on track.
//
// Returns:     True    Success
//              False   Otherwise
//
static bool dhcp_fsm () {
    // Enable reception of broadcast packets as some DHCP servers
    // use this to send responses. Use only in DHCP_STATE_INIT ???
    EtherCard::enableBroadcast();

    // We typically wait up to 20 seconds for an answer
    uint32_t end = millis() + DHCP_WAIT;
    while (dhcpState != DHCP_STATE_BOUND && millis() < end) {
        byte rc = DHCP_STATE_BAD;
        word len = 0;

        // We have no hardware link, so no further point
        if (!EtherCard::isLinkUp())
            continue;

        // Get a packet, and check it's ok. packetReceive returns the
        // sum of the source address (6), the destination address (6),
        // the type/length (2), and the data/padding (46-1500) fields.
        // The trailing CRC field (4) is dropped by the software.
        // packetLoop(), unfortunately, is strange and undocumented.
        if (dhcpState != DHCP_STATE_INIT) {
            len = EtherCard::packetReceive();
            // Reject inadequate packets
            if (len == 0 || EtherCard::packetLoop(len) > 0)
                continue;
#if 0
            // These are a waste of space
            // Reject ARP packets
            if (gPB[ETHTYPE_IP_H_V] == ETHTYPE_ARP_H_V &&
                        gPB[ETHTYPE_IP_L_V] == ETHTYPE_ARP_L_V)
                continue;
            // Reject ICMP packets (why are they here ???)
            if (gPB[IP_PROTO_P] == IP_PROTO_ICMP_V)
                continue;
#endif
            // Reject everything but UDP packets
            if (gPB[IP_PROTO_P] != IP_PROTO_UDP_V)
                continue;
        }

        // Switch between DHCP states.  This is a pretty
        // minimal DHCP state machine, but it should be
        // reliable, if slow.
        switch (dhcpState) {
        case DHCP_STATE_INIT:
            EtherCard::copyIp(EtherCard::myip, allZeros);
            EtherCard::copyIp(EtherCard::dhcpip, allZeros);
            currentXid = millis();
            currentXid = (currentXid << 16) + millis();
            dhcp_send(DHCP_MSG_DISCOVER, true);
            dhcpState = DHCP_STATE_SELECT;
            leaseStart = 0; // Set an invalid start time
            break;
        case DHCP_STATE_SELECT:
            if (check_for_dhcp_answer(len) == DHCP_STATE_SELECT) {
                parse_dhcpoffer(len);
                dhcp_send(DHCP_MSG_REQUEST, false);
                dhcpState = DHCP_STATE_REQUEST;
            }
            else
                dhcpState = DHCP_STATE_INIT;
            break;
        case DHCP_STATE_REQUEST:
            if (check_for_dhcp_answer(len) == DHCP_STATE_REQUEST) {
                dhcpState = DHCP_STATE_BOUND;
                leaseStart = millis();
            }
            else
                dhcpState = DHCP_STATE_INIT;
            break;
        case DHCP_STATE_BOUND:
            // Lease timed at 50%   :  move to DHCP_STATE_RENEW
            // We're only bounced out of this state by a timer.
            // Otherwise just hang on to the lease.
            break;
        case DHCP_STATE_RENEW:
            // Lease timed at 87.5% :  move to DHCP_STATE_REBIND
            // Otherwise just hang on to the lease if possible.
            rc = check_for_dhcp_answer(len);
            if (rc == DHCP_STATE_BOUND) {
                dhcpState = DHCP_STATE_BOUND;
                leaseStart = millis();
            }
            else if (rc == DHCP_STATE_INIT)
                dhcpState = DHCP_STATE_INIT;
            break;
        case DHCP_STATE_REBIND:
            // Lease timed at 100%  :  move to DHCP_STATE_INIT
            // Otherwise just hang on to the lease if possible.
            rc = check_for_dhcp_answer(len);
            if (rc == DHCP_STATE_BOUND) {
                dhcpState = DHCP_STATE_BOUND;
                leaseStart = millis();
            }
            else if (rc == DHCP_STATE_INIT)
                dhcpState = DHCP_STATE_INIT;
            break;
        // Never happen
        default:
            dhcpState = DHCP_STATE_INIT;
            break;
        }
    }

    EtherCard::disableBroadcast();

    // Did we get here with an IP or a timeout?
    if (EtherCard::myip[0] != 0) {
        if (EtherCard::gwip[0] != 0)
            EtherCard::setGwIp(EtherCard::gwip);
        return true;
    }

    return false;
}

// Update the DHCP lease if necessary.
//
// Returns:     True    Success
//              False   Otherwise
//
bool EtherCard::dhcpLease () {
    // If leaseStart is invalid, just quit
    if (leaseStart == 0)
        return false;

    // Find out when we are
    uint32_t now = millis();
    uint32_t renew = leaseStart + leaseTime / 2;
    uint32_t rebind = leaseStart + (leaseTime / 8 ) * 7;
    uint32_t restart = leaseStart + leaseTime;
    uint32_t newStart;
    byte rc = false;

    // The unsigned arithmetic makes rollover calculations tricky.
    // Re-assign leaseStart to (leaseStart + leaseTime) as soon as
    // the new tick is large enough. OTOH, rollover is ~50 days with
    // a msec tick - do we care ???
    if (now < leaseStart) {
        newStart = leaseTime - ((UINT32_MAX - leaseStart) % leaseTime);
        if (now < newStart) {
            if (now > leaseTime / 2)
                renew = newStart - leaseTime / 2;
            if (now > (leaseTime / 8))
                rebind = newStart - leaseTime / 8;
            restart = newStart;
        }
        else
            leaseStart = newStart;
    }

    // Update dhcpState according to the timers
    switch (dhcpState) {
    case DHCP_STATE_BAD:
    case DHCP_STATE_INIT:
    case DHCP_STATE_SELECT:
    case DHCP_STATE_REQUEST:
        break;
    case DHCP_STATE_BOUND:
        // At > 50% of the lease time,
        // move to DHCP_STATE_RENEW
        if (now > renew)
            dhcpState = DHCP_STATE_RENEW;
        break;
    case DHCP_STATE_RENEW:
        // At > 87.5% of the lease time,
        // move to DHCP_STATE_REBIND
        if (now > rebind)
            dhcpState = DHCP_STATE_REBIND;
        dhcp_send(DHCP_MSG_REQUEST, false);
        rc = true;
        break;
    case DHCP_STATE_REBIND:
        // At a 100% of the lease time,
        // move to DHCP_STATE_INIT
        if (now > restart)
            dhcpState = DHCP_STATE_INIT;
        else
            // This is a broadcast message.
            dhcp_send(DHCP_MSG_REQUEST, true);
        rc = true;
        break;
    default:
        // Never happen
        break;
    }

    // Call the main DHCP state machine
    // to handle the actual work.
    return rc ? dhcp_fsm() : true;
}

// Initialise DHCP with a particular name.
//
// Returns:     True    Success
//              False   Otherwise
//
bool EtherCard::dhcpSetup (const char *name) {
  hostname[0] = '\0';
  strncat(hostname, name, sizeof(hostname) - 1);
  dhcpState = DHCP_STATE_INIT;
  return dhcp_fsm();
}

// Initialise DHCP with the default name.
//
// Returns:     True    Success
//              False   Otherwise
//
bool EtherCard::dhcpSetup () {
  // Set a unique hostname, use Arduino-?? with last octet of mac address
  hostname[8] = (char) ('A' + (mymac[5] >> 4));
  hostname[9] = (char) ('A' + (mymac[5] & 0x0F));
  dhcpState = DHCP_STATE_INIT;
  return dhcp_fsm();
}

//
// eof
//

