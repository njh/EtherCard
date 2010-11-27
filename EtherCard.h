// This code slightly follows the conventions of, but is not derived from:
//      EHTERSHIELD_H library for Arduino etherShield
//      Copyright (c) 2008 Xing Yu.  All right reserved. (this is LGPL v2.1)
// It is however derived from the enc28j60 and ip code (which is GPL v2)
//      Title: Microchip ENC28J60 Ethernet Interface Driver
//      Author: Pascal Stang 
//      Modified by: Guido Socher
// Hence: GPL V2
//
// jcw, 2010-05-19

#ifndef EtherCard_h
#define EtherCard_h

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "enc28j60.h"
#include "ip_config.h"
#include "ip_arp_udp_tcp.h"
#include "net.h"
#include "dnslkup.h"
#include "websrv_help_functions.h"
#include <WProgram.h>

class BufferFiller : public Print {
    uint8_t *start, *ptr;
public:
    BufferFiller () {}
    BufferFiller (uint8_t* buf) : start (buf), ptr (buf) {}
        
    void emit_p(PGM_P fmt, ...);
    void emit_raw(const char* s, uint8_t len);
    
    uint8_t* buffer() const { return start; }
    uint16_t position() const { return ptr - start; }
    
    virtual void write(uint8_t v) { *ptr++ = v; }
};

struct EtherCard : ENC28J60, TCPIP, DNS, WebUtil {

    // lookup a host name via DNS, returns 1 if ok or 0 if this timed out
    // use during setup, as this discards all incoming requests until it returns
    byte dnsLookup (prog_char* name, byte *buffer, word length) {
        while (clientWaitingGw())
            packetLoop(buffer, packetReceive(buffer, length));
    
        dnsRequest(buffer, name);

        uint32_t start = millis();
        while (!dnsHaveAnswer()) {
            if (millis() - start >= 30000)
                return 0;
            word len = packetReceive(buffer, length);
            word pos = packetLoop(buffer, len);
            if (len != 0 && pos == 0)
                checkForDnsAnswer(buffer, length);
        }

        clientSetServerIp(dnsGetIp());
        return 1;
    }
    
};

#endif
