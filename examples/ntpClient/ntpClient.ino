/*

  UDP NTP Client using the EtherCard library

  Get the time from a Network Time Protocol (NTP) time server
  Demonstrates use of UDP sendPacket and ReceivePacket
  For more on NTP time servers and the messages needed to communicate with them,
  see http://en.wikipedia.org/wiki/Network_Time_Protocol

  created 13 Sep 2017
  by Jeroen Vermeulen
  inspired by https://www.arduino.cc/en/Tutorial/UdpNtpClient

  License: GPLv2

*/

#include <EtherCard.h>  // https://github.com/njh/EtherCard

// Ethernet mac address - must be unique on your network
const byte myMac[] PROGMEM = { 0x70, 0x69, 0x69, 0x2D, 0x30, 0x31 };
const char NTP_REMOTEHOST[] PROGMEM = "ntp.bit.nl";  // NTP server name
const unsigned int NTP_REMOTEPORT = 123;             // NTP requests are to port 123
const unsigned int NTP_LOCALPORT = 8888;             // Local UDP port to use
const unsigned int NTP_PACKET_SIZE = 48;             // NTP time stamp is in the first 48 bytes of the message
byte Ethernet::buffer[350];                          // Buffer must be 350 for DHCP to work

void setup() {
  Serial.begin(9600);
  Serial.println(F("\n[EtherCard NTP Client]"));

  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, myMac, SS) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

  if (!ether.dnsLookup(NTP_REMOTEHOST))
    Serial.println("DNS failed");

  uint8_t ntpIp[IP_LEN];
  ether.copyIp(ntpIp, ether.hisip);
  ether.printIp("NTP: ", ntpIp);

  ether.udpServerListenOnPort(&udpReceiveNtpPacket, NTP_LOCALPORT);
  Serial.println("Started listening for response.");

  sendNTPpacket(ntpIp);
}

void loop() {
  // this must be called for ethercard functions to work.
  ether.packetLoop(ether.packetReceive());
}

// send an NTP request to the time server at the given address
void sendNTPpacket(const uint8_t* remoteAddress) {
  // buffer to hold outgoing packet
  byte packetBuffer[ NTP_PACKET_SIZE];
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  ether.sendUdp(packetBuffer, NTP_PACKET_SIZE, NTP_LOCALPORT, remoteAddress, NTP_REMOTEPORT );
  Serial.println("NTP request sent.");
}

void udpReceiveNtpPacket(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *packetBuffer, uint16_t len) {
  Serial.println("NTP response received.");

  // the timestamp starts at byte 40 of the received packet and is four bytes,
  // or two words, long. First, extract the two words:
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSince1900 - seventyYears;

  // print Unix time:
  Serial.print("Unix time = ");
  Serial.println(epoch);
}

