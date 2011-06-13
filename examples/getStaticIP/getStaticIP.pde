// This demo does web requests to a fixed IP address, using a fixed gateway.
// 2010-11-27 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// ethernet interface mac address
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
// ethernet interface ip address
static byte myip[] = { 192,168,1,203 };
// gateway ip address
static byte gwip[] = { 192,168,1,1 };
// remote website ip address and port
static byte hisip[] = { 188,142,57,184 };

byte Ethernet::buffer[300];   // a very small tcp/ip buffer is enough here
static uint32_t timer;
static BufferFiller bfill;

// called to fill in a request to send out to the client
static word my_datafill_cb (byte fd) {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR("GET /blah HTTP/1.1\r\n"
                    "Host: jeefiles.equi4.com\r\n"
                    "\r\n"));
  return bfill.position();
}

// called when the client request is complete
static byte my_result_cb (byte fd, byte status, word off, word len) {
  Serial.print("<<< reply ");
  Serial.println((int) status);
  Serial.print((const char*) Ethernet::buffer + off);
  return 0;
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[getStaticIP]");
  
  ether.begin(sizeof Ethernet::buffer, mymac);
  ether.staticSetup(myip, gwip);
  ether.copyIp(ether.hisip, hisip);
  
  timer = -9999999; // start timing out right away
}

void loop () {
  ether.packetLoop(ether.packetReceive());
  if (ether.clientWaitingGw())
    return;
  
  if (millis() > timer) {
    timer = millis() + 5000;
    Serial.print(">>> REQ# ");
    byte id = ether.clientTcpReq(my_result_cb, my_datafill_cb, 80);
    Serial.println((int) id);
  }
}
