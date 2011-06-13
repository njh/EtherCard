// Demo using DHCP and DNS to perform a web client request.
// 2011-06-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[700];
static uint32_t timer;

static char website[] PROGMEM = "www.google.com";

// called when the client request is complete
static void my_callback (byte status, word off, word len) {
  Serial.println(">>>");
  Ethernet::buffer[off+300] = 0;
  Serial.print((const char*) Ethernet::buffer + off);
  Serial.println("...");
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[webClient]");

  ether.begin(sizeof Ethernet::buffer, mymac);
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIP("IP:  ", ether.myip);
  ether.printIP("GW:  ", ether.gwip);  
  ether.printIP("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIP("SRV: ", ether.hisip);
}

void loop () {
  ether.packetLoop(ether.packetReceive());
  
  if (millis() > timer) {
    timer = millis() + 5000;
    Serial.println();
    Serial.print("<<< REQ ");
    ether.browseUrl(PSTR("/foo/"), "bar", website, my_callback);
  }
}
