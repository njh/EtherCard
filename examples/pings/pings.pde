// Ping a remote server, also uses DHCP and DNS.
// 2011-06-12 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[700];
static uint32_t timer;

static char website[] PROGMEM = "www.google.com";

// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  ether.printIP(">>> ping from: ", ptr);
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[pings]");
  
  ether.begin(sizeof Ethernet::buffer, mymac);
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIP("IP: ", ether.myip);
  ether.printIP("GW: ", ether.gwip);

  // use DNS to locate the IP address we want to ping
  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
  // ether.hisip[0] = 74;
  // ether.hisip[1] = 125;
  // ether.hisip[2] = 77;
  // ether.hisip[3] = 99;
  ether.printIP("Server: ", ether.hisip);
    
  // call this to report others pinging us
  ether.registerPingCallback(gotPinged);
  
  timer = -9999999; // start timing out right away
  Serial.println();
}

void loop () {
  word len = ether.packetReceive(); // go receive new packets
  word pos = ether.packetLoop(len); // respond to incoming pings
  
  // report whenever a reply to our outgoing ping comes back
  if (len > 0 && ether.packetLoopIcmpCheckReply(ether.hisip)) {
    Serial.print("  ");
    Serial.print((micros() - timer) * 0.001, 3);
    Serial.println(" ms");
  }
  
  // ping a remote server once every few seconds
  if (micros() - timer >= 5000000) {
    ether.printIP("Pinging: ", ether.hisip);
    timer = micros();
    ether.clientIcmpRequest(ether.hisip);
  }
}
