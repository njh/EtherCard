// This is a demo of the RBBB running as webserver with the Ether Card
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x54,0x55,0x58,0x10,0x00,0x26 };
static byte myip[] = { 192,168,1,203 };

byte gPacketBuffer[500];
EtherCard eth (sizeof gPacketBuffer);

void setup () {
    eth.spiInit();
    eth.initialize(mymac);
    eth.initIp(mymac, myip, 80);
}

static word homePage() {
    long t = millis() / 1000;
    word h = t / 3600;
    byte m = (t / 60) % 60;
    byte s = t % 60;
    BufferFiller bfill = eth.tcpOffset();
    bfill.emit_p(PSTR(
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Pragma: no-cache\r\n"
        "\r\n"
        "<meta http-equiv='refresh' content='1'/>"
        "<title>RBBB server</title>" 
        "<h1>$D$D:$D$D:$D$D</h1>"),
            h/10, h%10, m/10, m%10, s/10, s%10);
    return bfill.position();
}

void loop () {
    word len = eth.packetReceive();
    word pos = eth.packetLoop(len);
    
    if (pos)  // check if valid tcp data is received
        eth.httpServerReply(homePage()); // send web page data
}
