// Demo using DHCP and DNS to perform a web client request.
// 2011-06-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

static byte buf[700];
static DHCPinfo dhcp;
static EtherCard eth;
static uint32_t timer;

static char hisname[] PROGMEM = "www.google.com";

// called when the client request is complete
static void my_callback (byte status, word off, word len) {
    Serial.println(">>>");
    buf[off+300] = 0;
    Serial.print((const char*) buf + off);
    Serial.println("...");
}

void setup () {
    Serial.begin(57600);
    Serial.println("\n[webClient]");
    
    eth.spiInit();
    Serial.print("ENC28J60 rev ");
    Serial.println(eth.initialize(mymac), HEX);
    
    eth.dhcpInit(mymac, dhcp);
    while (!eth.dhcpCheck(buf, eth.packetReceive(buf, sizeof buf - 1)))
        ;
    Serial.println("DHCP ok.");
    
    eth.initIp(mymac, dhcp.myip, 0);
    eth.clientSetGwIp(dhcp.gwip);

    if (eth.dnsLookup(hisname, buf, sizeof buf))
        Serial.println("DNS ok.");
}

void loop () {
    word len = eth.packetReceive(buf, sizeof buf);
    word pos = eth.packetLoop(buf, len);
    
    if (millis() > timer) {
        timer = millis() + 5000;
        Serial.println();
        Serial.print("<<< REQ ");
        eth.browseUrl(PSTR("/foo/"), "bar", hisname, my_callback);
    }
}
