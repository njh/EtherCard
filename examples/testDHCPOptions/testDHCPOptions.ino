// Arduino demo sketch for testing the custom options in the DHCP client
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include <EtherCard.h>

static byte mymac[] = { 0x6e, 0x1b, 0xd0, 0x2e, 0xdd, 0xa5 };

byte Ethernet::buffer[700];


void dhcpOptionCallback(uint8_t option, const byte* data, uint8_t len) {
  if (option == 42) {
    ether.printIp("NTP IP: ", data);
  }
}

void setup () {
  Serial.begin(57600);
  Serial.println(F("\n[testDHCP]"));

  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i) {
    Serial.print(mymac[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();
  
  if (ether.begin(sizeof Ethernet::buffer, mymac, 8) == 0) 
    Serial.println(F("Failed to access Ethernet controller"));

  // Add callback for the NTP option (DHCP Option number 42)
  ether.dhcpAddOptionCallback(42, dhcpOptionCallback);

  Serial.println(F("Setting up DHCP"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));
  
  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
}

void loop () {
  ether.packetLoop(ether.packetReceive());
}
