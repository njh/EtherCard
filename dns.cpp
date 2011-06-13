// DNS look-up functions based on the udp client
// Author: Guido Socher 
// Copyright: GPL V2
//
// Mods bij jcw, 2010-05-20

#include "EtherCard.h"
#include "net.h"

#define gPB gPacketBuffer

static byte dnstid_l; // a counter for transaction ID
#define DNSCLIENT_SRC_PORT_H 0xE0 
static byte dnsip[] = {8,8,8,8}; // the google public DNS, don't change unless there is a real need
static byte dns_answerip[4];

static void dnsRequest (const prog_char *progmem_hostname) {
  ++dnstid_l; // increment for next request, finally wrap
  EtherCard::udpPrepare((DNSCLIENT_SRC_PORT_H << 8) | dnstid_l, dnsip, 53);
  memset(gPB + UDP_DATA_P, 0, 12);
  
  byte *p = gPB + UDP_DATA_P + 12;
  char c;
  do {
    byte n = 0;
    for(;;) {
      c = pgm_read_byte(progmem_hostname++);
      if (c == '.' || c == 0)
        break;
      p[++n] = c;
    }
    *p++ = n;
    p += n;
  } while (c != 0);
  
  *p++ = 0; // terminate with zero, means root domain.
  *p++ = 0;
  *p++ = 1; // type A
  *p++ = 0; 
  *p++ = 1; // class IN
  byte i = p - gPB - UDP_DATA_P;
  gPB[UDP_DATA_P] = i;
  gPB[UDP_DATA_P+1] = dnstid_l;
  gPB[UDP_DATA_P+2] = 1; // flags, standard recursive query
  gPB[UDP_DATA_P+5] = 1; // 1 question
  EtherCard::udpTransmit(i);
}

static void checkForDnsAnswer (uint16_t plen) {
  byte *p = gPB + UDP_DATA_P;
  if (plen < 70 || gPB[UDP_SRC_PORT_L_P] != 53 ||
                   gPB[UDP_DST_PORT_H_P] != DNSCLIENT_SRC_PORT_H ||
                   gPB[UDP_DST_PORT_L_P] != dnstid_l ||
                   p[1] != dnstid_l ||
                   (p[3] & 0x8F) != 0x80) 
    return;
    
  p += *p; // we encoded the query len into tid
  while (1) {
    if (*p & 0xC0)
      p += 2;
    else
      while (p < gPB + plen - 7) {
        if (*++p == 0) {
          ++p;
          break;
        }
      }
    if (p[1] == 1)     // check type == 1 for "A"
      break;
    p += 2 + 2 + 4;    // skip type & class & TTL
    p += *p + 2;    // skip datalength bytes
  }
  
  if (p[9] == 4) // only if IPv4
    EtherCard::copy4(dns_answerip, p + 10);
}

// lookup a host name via DNS, returns 1 if ok or 0 if this timed out
// use during setup, as this discards all incoming requests until it returns
const byte* EtherCard::dnsLookup (prog_char* name) {
  while (clientWaitingGw())
    packetLoop(packetReceive());
    
  memset(dns_answerip, 0, 4);
  dnsRequest(name);

  word start = millis();
  while (dns_answerip[0] == 0) {
    if ((word) (millis() - start) >= 30000)
      return 0;
    word len = packetReceive();
    if (len > 0 && packetLoop(len) == 0)
      checkForDnsAnswer(len);
  }

  return dns_answerip;
}
