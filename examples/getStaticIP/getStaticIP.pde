// This demo does web requests to a fixed IP address, using a fixed gateway.
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
// remote website ip address and port
static byte hisip[4] = { 80,190,241,133 };
static word hisport = 80;

EtherCard eth;
MilliTimer requestTimer;

static byte buf[300];   // a very small tcp/ip buffer is enough here

// called to fill in a request to send out to the client
static word my_datafill_cb (byte fd) {
    BufferFiller bfill = eth.tcpOffset(buf);
    bfill.emit_p(PSTR("GET /blah HTTP/1.1\r\n"
                      "Host: jeefiles.equi4.com\r\n"
                      "\r\n"));
    return bfill.position();
}

// called when the client request is complete
static byte my_result_cb (byte fd, byte status, word off, word len) {
    Serial.print("<<< reply ");
    Serial.println((int) status);
    Serial.print((const char*) buf + off);
    return 0;
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[getStaticIP]");
    
    eth.spiInit();
    eth.initialize(mymac);
    eth.initIp(mymac, myip, 0);
    eth.clientSetGwIp(gwip);    // outgoing requests need a gateway
    eth.clientSetServerIp(hisip);
    
    requestTimer.set(1); // send first request as soon as possible
}

void loop () {
    word len = eth.packetReceive(buf, sizeof buf);
    word pos = eth.packetLoop(buf, len);
    if (eth.clientWaitingGw())
        return;
    
    if (requestTimer.poll(5000)) {
        Serial.print(">>> REQ# ");
        byte id = eth.clientTcpReq(my_result_cb, my_datafill_cb, hisport);
        Serial.println((int) id);
    }
}
