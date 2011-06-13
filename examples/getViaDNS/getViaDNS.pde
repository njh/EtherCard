// This demo does web requests via DNS lookup, using a fixed gateway.
// 2010-11-27 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>
#include <Ports.h>
#include <RF12.h> // needed to avoid a linker error :(

// ethernet interface mac address
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
// ethernet interface ip address
static byte myip[] = { 192,168,1,203 };
// gateway ip address
static byte gwip[] = { 192,168,1,1 };
// remote website name
static char website[] PROGMEM = "jeefiles.equi4.com";

#define REQUEST_RATE 5000 // milliseconds

byte Ethernet::buffer[300];   // a very small tcp/ip buffer is enough here
MilliTimer requestTimer;

// called when the client request is complete
static void my_result_cb (byte status, word off, word len) {
    Serial.print("<<< ");
    Serial.print(REQUEST_RATE - requestTimer.remaining());
    Serial.println(" ms");
    Serial.print((const char*) Ethernet::buffer + off);
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[getViaDNS]");
    
    ether.begin(sizeof Ethernet::buffer, mymac);
    ether.staticSetup(myip, gwip);

    if (!ether.dnsLookup(website))
      Serial.println("DNS failed");

    ether.printIp("Server: ", ether.hisip);
    
    requestTimer.set(1); // send first request as soon as possible
}

void loop () {
    ether.packetLoop(ether.packetReceive());
    
    if (requestTimer.poll(REQUEST_RATE)) {
        Serial.println(">>> REQ");
        ether.browseUrl(PSTR("/foo/"), "bar", website, my_result_cb);
    }
}
