// This demo does web requests to a fixed IP address, using a fixed gateway.
// 2010-11-27 <jc@wippler.nl>
//
// License: GPLv2

#include <EtherCard.h>

#define REQUEST_RATE 5000 // milliseconds

// Ethernet interface MAC address
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
// Ethernet interface IP address
static byte myip[] = { 192,168,1,203 };
// Ethernet interface IP netmask
static byte mask[] = { 255,255,255,0 };
// Gateway IP address
static byte gwip[] = { 192,168,1,1 };
// Remote website IP address and port
static byte hisip[] = { 74,125,79,99 };
// Remote website name
const char website[] PROGMEM = "google.com";

byte Ethernet::buffer[300];   // a very small TCP/IP buffer is enough here
static long timer;

// Called when the client request is complete
static void my_result_cb (byte status, word off, word len) {
  Serial.print("<<< reply ");
  Serial.print(millis() - timer);
  Serial.println(" ms");
  Serial.println((const char*) Ethernet::buffer + off);
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[getStaticIP]");

  // Change 'SS' to your Slave Select pin if you aren't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
    Serial.println( "Failed to access Ethernet controller");

  ether.staticSetup(myip, gwip, NULL, mask);

  ether.copyIp(ether.hisip, hisip);
  ether.printIp("Server: ", ether.hisip);

  while (ether.clientWaitingGw())
    ether.packetLoop(ether.packetReceive());
  Serial.println("Gateway found");

  timer = - REQUEST_RATE; // start timing out right away
}

void loop () {
  ether.packetLoop(ether.packetReceive());

  if (millis() > timer + REQUEST_RATE) {
    timer = millis();
    Serial.println("\n>>> REQ");
    ether.browseUrl(PSTR("/foo/"), "bar", website, my_result_cb);
  }
}
