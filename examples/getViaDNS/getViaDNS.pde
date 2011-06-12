// This demo does web requests via DNS lookup, using a fixed gateway.
// 2010-11-27 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>
#include <Ports.h>
#include <RF12.h> // needed to avoid a linker error :(

// ethernet interface mac address
static byte mymac[] = { 0x54,0x55,0x58,0x10,0x00,0x26 };
// ethernet interface ip address
static byte myip[] = { 192,168,1,203 };
// gateway ip address
static byte gwip[] = { 192,168,1,1 };
// remote website name
static char website[] PROGMEM = "jeefiles.equi4.com";

#define REQUEST_RATE 5000 // milliseconds

byte gPacketBuffer[300];   // a very small tcp/ip buffer is enough here
EtherCard eth (sizeof gPacketBuffer);
MilliTimer requestTimer;

// called when the client request is complete
static void my_result_cb (byte status, word off, word len) {
    Serial.print("<<< ");
    Serial.print(REQUEST_RATE - requestTimer.remaining());
    Serial.println(" ms");
    Serial.print((const char*) gPacketBuffer + off);
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[getViaDNS]");
    
    eth.spiInit();
    eth.initialize(mymac);
    eth.initIp(mymac, myip, 0);
    eth.clientSetGwIp(gwip);

    const byte* ipaddr = eth.dnsLookup(website);
    eth.printIP("Server: ", ipaddr);
    eth.clientSetServerIp(ipaddr);
    
    requestTimer.set(1); // send first request as soon as possible
}

void loop () {
    eth.packetLoop(eth.packetReceive());
    
    if (requestTimer.poll(REQUEST_RATE)) {
        Serial.println(">>> REQ");
        eth.browseUrl(PSTR("/foo/"), "bar", website, my_result_cb);
    }
}
