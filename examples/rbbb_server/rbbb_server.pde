// This is a demo of the RBBB running as webserver with the Ether Card
// 2010-05-28 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address
static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

// ethernet interface ip address
static byte myip[4] = { 192,168,178,203 };

// listen port for tcp/www:
#define HTTP_PORT 80

static byte buf[500];
static BufferFiller bfill;

EtherCard eth;

void setup () {
    eth.spiInit();
    eth.initialize(mymac);
    eth.initIp(mymac, myip, HTTP_PORT);
}

static void homePage() {
    long t = millis() / 1000;
    word h = t / 3600;
    byte m = (t / 60) % 60;
    byte s = t % 60;
    bfill.emit_p(PSTR(
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Pragma: no-cache\r\n"
        "\r\n"
        "<meta http-equiv='refresh' content='1'/>"
        "<title>RBBB server</title>" 
        "<h1>$D$D:$D$D:$D$D</h1>"), h/10, h%10, m/10, m%10, s/10, s%10);
}

void loop () {
    word len = eth.packetReceive(buf, sizeof buf);
    // ENC28J60 loop runner: handle ping and wait for a tcp packet
    word pos = eth.packetLoop(buf,len);
    // check if valid tcp data is received
    if (pos) {
        bfill = eth.tcpOffset(buf);
        homePage();
        eth.httpServerReply(buf,bfill.position()); // send web page data
    }
}
