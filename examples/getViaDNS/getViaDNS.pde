// This demo does web requests via DNS lookup, using a fixed gateway.
// 2010-11-27 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>
#include <Ports.h>
#include <RF12.h> // needed to avoid a linker error :(

// ethernet interface mac address
static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };
// ethernet interface ip address
static byte myip[4] = { 192,168,178,203 };
// gateway ip address
static byte gwip[4] = { 192,168,178,1 };
// remote website name
static char hisname[] PROGMEM = "jeefiles.equi4.com";

#define REQUEST_RATE 5000 // milliseconds

EtherCard eth;
MilliTimer requestTimer;

static byte buf[300];   // a very small tcp/ip buffer is enough here

// called when the client request is complete
static void my_result_cb (byte status, word off, word len) {
    Serial.print("<<< ");
    Serial.print(REQUEST_RATE - requestTimer.remaining());
    Serial.println(" ms");
    Serial.print((const char*) buf + off);
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[getViaDNS]");
    
    eth.spiInit();
    eth.initialize(mymac);
    eth.initIp(mymac, myip, 0);
    eth.clientSetGwIp(gwip);    // outgoing requests need a gateway

    if (!eth.dnsLookup(hisname, buf, sizeof buf))
        Serial.println("DNS lookup failed");
    
    requestTimer.set(1); // send first request as soon as possible
}

void loop () {
    eth.packetLoop(buf, eth.packetReceive(buf, sizeof buf));
    
    if (requestTimer.poll(REQUEST_RATE)) {
        Serial.println(">>> REQ");
        eth.browseUrl(PSTR("/foo/"), "bar", hisname, my_result_cb);
    }
}
