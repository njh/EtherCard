// Ping a remote server, also uses DHCP and DNS.
// 2011-06-12 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[700];
static uint32_t timer;

// called when a ping comes in (replies to it are automatic)
static void gotPinged (byte* ptr) {
  ether.printIp(">>> ping from: ", ptr);
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[pings]");
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);

#if 1
  // use DNS to locate the IP address we want to ping
  if (!ether.dnsLookup(PSTR("www.google.com")))
    Serial.println("DNS failed");
#else
  ether.parseIp(ether.hisip, "74.125.77.99");
#endif
  ether.printIp("SRV: ", ether.hisip);
    
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
    ether.printIp("Pinging: ", ether.hisip);
    timer = micros();
    ether.clientIcmpRequest(ether.hisip);
  }
}
