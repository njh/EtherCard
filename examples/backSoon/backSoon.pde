// Present a "Will be back soon web page", as stand-in webserver.
// jcw, 2011-01-30
 
#include <EtherCard.h>

// ethernet mac address - must be unique on your network
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

byte Ethernet::buffer[1000]; // tcp/ip send and receive buffer
static BufferFiller bfill;

char page[] PROGMEM =
"HTTP/1.0 503 Service Unavailable\r\n"
"Content-Type: text/html\r\n"
"Retry-After: 600\r\n"
"\r\n"
"<html>"
  "<head><title>"
    "Service Temporarily Unavailable"
  "</title></head>"
  "<body>"
    "<h3>This service is currently unavailable</h3>"
    "<p><em>"
      "The main server is currently off-line.<br />"
      "Please try again later."
    "</em></p>"
    "<p>-jcw</p>"
  "</body>"
"</html>"
;

void setup(){
    ether.begin(sizeof Ethernet::buffer, mymac);
    if (!ether.dhcpSetup())
      Serial.println("DHCP failed");

    ether.printIP("IP: ", ether.myip);
    ether.initIp(ether.myip, 80);
}

void loop(){
    word len = ether.packetReceive();
    word pos = ether.packetLoop(len);
    
    if (pos) {
        bfill = ether.tcpOffset();
        bfill.emit_p(page);
        ether.httpServerReply(bfill.position());
    }
}
