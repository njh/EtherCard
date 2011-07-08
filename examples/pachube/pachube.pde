// Simple demo for feeding some random data to Pachube.
// 2011-07-08 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

#include <EtherCard.h>

// change these settings to match your own setup
#define FEED    5942
#define APIKEY  "bXkPFCiYm57f7flLyD86bm0HK3TXsfuQF-Jeyh3HeMg"

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

char website[] PROGMEM = "api.pachube.com";

byte Ethernet::buffer[700];
uint32_t timer;

static void my_datafill (BufferFiller& buf) {
  // generate the header
  buf.emit_p(PSTR("GET http://$F/v2/feeds/$D.csv HTTP/1.0" "\r\n"
                  "Host: $F" "\r\n"
                  "X-PachubeApiKey: $F" "\r\n"
                   "\r\n"), website, FEED, website, PSTR(APIKEY));
  // send two fake values as payload
  buf.print("0,");
  buf.println(millis() / 123);
  buf.print("1,");
  buf.println(micros() / 123456);
}

//------------------------------------------------------------------------------
// the following code needs to be moved into the EtherCard library

static void (*df_fun)(BufferFiller&);

static word datafill (byte fd) {
  BufferFiller buf = EtherCard::tcpOffset();
  df_fun(buf);
  return buf.position();
}

static void tcpRequest (word port, void (*df)(BufferFiller&)) {
  df_fun = df;
  ether.clientTcpReq(0, datafill, 80);
}

//------------------------------------------------------------------------------

void setup () {
  Serial.begin(57600);
  Serial.println("\n[webClient]");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);
}

void loop () {
  ether.packetLoop(ether.packetReceive());
  
  if (millis() > timer) {
    timer = millis() + 10000;
    tcpRequest(80, my_datafill);
  }
}
