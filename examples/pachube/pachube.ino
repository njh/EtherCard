// Simple demo for feeding some random data to Pachube.
// 2011-07-08 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

#include <EtherCard.h>

// change these settings to match your own setup
#define FEED    "5942"
#define APIKEY  "bXkPFCiYm57f7flLyD86bm0HK3TXsfuQF-Jeyh3HeMg"

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

char website[] PROGMEM = "api.pachube.com";

byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;

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
    
    // generate two fake values as payload - by using a separate stash,
    // we can determine the size of the generated message ahead of time
    byte sd = stash.create();
    stash.print("0,");
    stash.println((word) millis() / 123);
    stash.print("1,");
    stash.println((word) micros() / 456);
    stash.save();
    
    // generate the header with payload - note that the stash size is used,
    // and that a "stash descriptor" is passed in as argument using "$H"
    Stash::prepare(PSTR("PUT http://$F/v2/feeds/$F.csv HTTP/1.0" "\r\n"
                        "Host: $F" "\r\n"
                        "X-PachubeApiKey: $F" "\r\n"
                        "Content-Length: $D" "\r\n"
                        "\r\n"
                        "$H"),
            website, PSTR(FEED), website, PSTR(APIKEY), stash.size(), sd);

    // send the packet - this also releases all stash buffers once done
    ether.tcpSend();
  }
}
