// Ping a remote server, also uses DHCP and DNS.
// 2011-06-12 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

byte gPacketBuffer[700];
static EtherCard eth (sizeof gPacketBuffer);
static DHCPinfo dhcp;
static uint32_t timer;

static char website[] PROGMEM = "www.google.com";
static byte dest[4];

// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  eth.printIP(">>> ping from: ", ptr);
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[pings]");
  
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
  eth.copy4(dest, eth.dnsLookup(website));
  // dest[0] = 74; dest[1] = 125; dest[2] = 77; dest[3] = 99;
  eth.printIP("Server: ", dest);
    
  // call this to report others pinging us
  eth.registerPingCallback(gotPinged);
  
  timer = -9999999; // start timing out right away
  Serial.println();
}

void loop () {
  word len = eth.packetReceive(); // go receive new packets
  word pos = eth.packetLoop(len); // respond to incoming pings
  
  // report whenever a reply to our outgoing ping comes back
  if (len > 0 && eth.packetLoopIcmpCheckReply(dest)) {
    Serial.print("  ");
    Serial.print((micros() - timer) * 0.001, 3);
    Serial.println(" ms");
  }
  
  // ping a remote server once every few seconds
  if (micros() - timer >= 5000000) {
    eth.printIP("Pinging: ", dest);
    timer = micros();
    eth.clientIcmpRequest(dest);
  }
}
