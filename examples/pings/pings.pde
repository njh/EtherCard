// Ping a remote server, also uses DHCP and DNS.
// 2011-06-12 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

byte gPacketBuffer[700];
static EtherCard eth (sizeof gPacketBuffer);
static DHCPinfo dhcp;
static uint32_t timer;

static char website[] PROGMEM = "www.google.com";

// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  eth.printIP(">>> ping from: ", ptr);
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[webClient]");
  
  // boilerplate to set things up with a DHCP request
  eth.spiInit();
  eth.dhcpInit(mymac, dhcp);
  while (!eth.dhcpCheck(eth.packetReceive()))
      ;
  eth.printIP("IP: ", dhcp.myip);
  eth.printIP("GW: ", dhcp.gwip);
  eth.initIp(mymac, dhcp.myip, 0);
  eth.clientSetGwIp(dhcp.gwip);

  // use DNS to locate the IP address we want to ping
  if (eth.dnsLookup(website))
      eth.printIP("Website: ", eth.dnsGetIp());
    
  // call this to report others pinging us
  eth.registerPingCallback(gotPinged);
}

void loop () {
  word len = eth.packetReceive(); // go receive new packets
  word pos = eth.packetLoop(len); // respond to incoming pings
  
  // report whenever a reply to our outgoing ping comes back
  if (len > 0 && eth.packetLoopIcmpCheckReply(eth.dnsGetIp()))
    Serial.println("  alive!");
  
  // ping a remote server once every few seconds
  if (millis() > timer) {
    timer = millis() + 5000;
    eth.printIP("Pinging: ", eth.dnsGetIp());
    eth.clientIcmpRequest(eth.dnsGetIp());
  }
}
