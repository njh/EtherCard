// Demo using DHCP and DNS to perform a web client request.
// 2011-06-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

byte gPacketBuffer[700];
static EtherCard eth (sizeof gPacketBuffer);
static DHCPinfo dhcp;
static uint32_t timer;

static char website[] PROGMEM = "www.google.com";

// called when the client request is complete
static void my_callback (byte status, word off, word len) {
  Serial.println(">>>");
  gPacketBuffer[off+300] = 0;
  Serial.print((const char*) gPacketBuffer + off);
  Serial.println("...");
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[webClient]");
  
  eth.spiInit();
  Serial.print("Ethernet controller rev ");
  Serial.println(eth.dhcpInit(mymac, dhcp), HEX);
  while (!eth.dhcpCheck(eth.packetReceive()))
      ;
  eth.printIP("IP: ", dhcp.myip);
  eth.printIP("GW: ", dhcp.gwip);
  
  eth.initIp(mymac, dhcp.myip, 0);
  eth.clientSetGwIp(dhcp.gwip);

  if (eth.dnsLookup(website))
    eth.printIP("Website: ", eth.dnsGetIp());
}

void loop () {
  word len = eth.packetReceive();
  word pos = eth.packetLoop(len);
  
  if (millis() > timer) {
    timer = millis() + 5000;
    Serial.println();
    Serial.print("<<< REQ ");
    eth.browseUrl(PSTR("/foo/"), "bar", website, my_callback);
  }
}
