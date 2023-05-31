/*
  Demo receiving WOL magic packets on Arduino, it can be used to send push notifications to Arduino.
  rpm based linux: ether-wake 42:41:52:52:59:33
  deb based linux: etherwake -i enp1s0 42:41:52:52:59:33  (replace enp1s0 with your network card id)
  
*/

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x42, 0x41, 0x52, 0x52, 0x59, 0x33 };  //ASCII: BARRY1
byte Ethernet::buffer[700];
const char hostname[] = "device.example.com";

static uint32_t timer;

void setup () {
  Serial.begin(115200);
  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup(hostname, true))
  {
    Serial.println(F("DHCP failed"));
  }

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);
}

void loop () {
  ether.packetLoop(ether.packetReceive());

  if (ether.pollWOLInterrupt())
  {
    wolReceived();
  }
}

/* Method called when a WOL magic packet is received
*/
void wolReceived()
{
  ether.clearWOLInterrupt();
  Serial.println(F("Magic Packet received!"));
}
