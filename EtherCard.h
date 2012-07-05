// This code slightly follows the conventions of, but is not derived from:
//      EHTERSHIELD_H library for Arduino etherShield
//      Copyright (c) 2008 Xing Yu.  All right reserved. (this is LGPL v2.1)
// It is however derived from the enc28j60 and ip code (which is GPL v2)
//      Author: Pascal Stang 
//      Modified by: Guido Socher
//      DHCP code: Andrew Lindsay
// Hence: GPL V2
//
// 2010-05-19 <jc@wippler.nl>

#ifndef EtherCard_h
#define EtherCard_h


#if ARDUINO >= 100
  #include <Arduino.h> // Arduino 1.0
  #define WRITE_RESULT size_t
  #define WRITE_RETURN return 1;
#else
  #include <WProgram.h> // Arduino 0022
  #define WRITE_RESULT void
  #define WRITE_RETURN
#endif

#include <avr/pgmspace.h>
#include "enc28j60.h"
#include "net.h"

typedef struct {
  uint8_t count;     // number of allocated pages
  uint8_t first;     // first allocated page
  uint8_t last;      // last allocated page
} StashHeader;

class Stash : public /*Stream*/ Print, private StashHeader {
  uint8_t curr;      // current page
  uint8_t offs;      // current offset in page
  
  typedef struct {
    union {
      uint8_t bytes[64];
      uint16_t words[32];
      struct {
        StashHeader head;
        uint8_t filler[59];
        uint8_t tail;
        uint8_t next;
      };
    };
    uint8_t bnum;
  } Block;

  static uint8_t allocBlock ();
  static void freeBlock (uint8_t block);
  static uint8_t fetchByte (uint8_t blk, uint8_t off);

  static Block bufs[2];
  static uint8_t map[256/8];

public:
  static void initMap (uint8_t last);
  static void load (uint8_t idx, uint8_t blk);
  static uint8_t freeCount ();

  Stash () : curr (0) { first = 0; }
  Stash (uint8_t fd) { open(fd); }
  
  uint8_t create ();
  uint8_t open (uint8_t blk);
  void save ();
  void release ();

  void put (char c);
  char get ();
  uint16_t size ();

  virtual WRITE_RESULT write(uint8_t b) { put(b); WRITE_RETURN }
  
  // virtual int available() {
  //   if (curr != last)
  //     return 1;
  //   load(1, last);
  //   return offs < bufs[1].tail;
  // }
  // virtual int read() {
  //   return available() ? get() : -1;      
  // }
  // virtual int peek() {
  //   return available() ? bufs[1].bytes[offs] : -1;      
  // }
  // virtual void flush() {
  //   curr = last;
  //   offs = 63;
  // }
  
  static void prepare (PGM_P fmt, ...);
  static uint16_t length ();
  static void extract (uint16_t offset, uint16_t count, void* buf);
  static void cleanup ();

  friend void dumpBlock (const char* msg, uint8_t idx); // optional
  friend void dumpStash (const char* msg, void* ptr);   // optional
};

class BufferFiller : public Print {
  uint8_t *start, *ptr;
public:
  BufferFiller () {}
  BufferFiller (uint8_t* buf) : start (buf), ptr (buf) {}
      
  void emit_p (PGM_P fmt, ...);
  void emit_raw (const char* s, uint16_t n) { memcpy(ptr, s, n); ptr += n; }
  void emit_raw_p (PGM_P p, uint16_t n) { memcpy_P(ptr, p, n); ptr += n; }
  
  uint8_t* buffer () const { return start; }
  uint16_t position () const { return ptr - start; }
  
  virtual WRITE_RESULT write (uint8_t v) { *ptr++ = v; WRITE_RETURN }
};

class EtherCard : public Ethernet {
public:
  static uint8_t mymac[6];  // my MAC address
  static uint8_t myip[4];   // my ip address
  static uint8_t mymask[4]; // my net mask
  static uint8_t gwip[4];   // gateway
  static uint8_t dhcpip[4]; // dhcp server
  static uint8_t dnsip[4];  // dns server
  static uint8_t hisip[4];  // dns result
  static uint16_t hisport;  // tcp port to connect to (default 80)
  // EtherCard.cpp
  static uint8_t begin (const uint16_t size, const uint8_t* macaddr,
                        uint8_t csPin =8);  
  static bool staticSetup (const uint8_t* my_ip =0,
                            const uint8_t* gw_ip =0,
                             const uint8_t* dns_ip =0);
  // tcpip.cpp
  static void initIp (uint8_t *myip,uint16_t wwwp);
  static void makeUdpReply (char *data,uint8_t len, uint16_t port);
  static uint16_t packetLoop (uint16_t plen);
  static void httpServerReply (uint16_t dlen);
  static void setGwIp (const uint8_t *gwipaddr);
  static uint8_t clientWaitingGw ();
  static uint8_t clientTcpReq (uint8_t (*r)(uint8_t,uint8_t,uint16_t,uint16_t),
                               uint16_t (*d)(uint8_t),uint16_t port);
  static void browseUrl (prog_char *urlbuf, const char *urlbuf_varpart,
                         prog_char *hoststr,
                         void (*cb)(uint8_t,uint16_t,uint16_t));
  static void httpPost (prog_char *urlbuf, prog_char *hoststr,
                        prog_char *header, const char *postval,
                        void (*cb)(uint8_t,uint16_t,uint16_t));
  static void ntpRequest (uint8_t *ntpip,uint8_t srcport);
  static uint8_t ntpProcessAnswer (uint32_t *time, uint8_t dstport_l);
  static void udpPrepare (uint16_t sport, uint8_t *dip, uint16_t dport);
  static void udpTransmit (uint16_t len);
  static void sendUdp (char *data,uint8_t len,uint16_t sport,
                                              uint8_t *dip, uint16_t dport);
  static void registerPingCallback (void (*cb)(uint8_t*));
  static void clientIcmpRequest (const uint8_t *destip);
  static uint8_t packetLoopIcmpCheckReply (const uint8_t *ip_mh);
  static void sendWol (uint8_t *wolmac);
  // new stash-based API
  static uint8_t tcpSend ();
  static const char* tcpReply (byte fd);
  // dhcp.cpp
  static uint32_t dhcpStartTime ();
  static uint32_t dhcpLeaseTime ();
  static byte dhcpFSM ();
  static bool dhcpValid ();
  static bool dhcpLease ();
  static bool dhcpSetup (const char *);
  static bool dhcpSetup ();
  // dns.cpp
  static bool dnsLookup (prog_char* name, bool fromRam =false);
  // webutil.cpp
  static void copyIp (uint8_t *dst, const uint8_t *src);
  static void copyMac (uint8_t *dst, const uint8_t *src);
  static void printIp (const char* msg, const uint8_t *buf);
  static uint8_t findKeyVal(const char *str,char *strbuf,
                            uint8_t maxlen, const char *key);
  static void urlDecode(char *urlbuf);
  static  void urlEncode(char *str,char *urlbuf);
  static uint8_t parseIp(uint8_t *bytestr,char *str);
  static void makeNetStr(char *rs,uint8_t *bs,uint8_t len,
                                              char sep,uint8_t base);
};

extern EtherCard ether;

#endif
