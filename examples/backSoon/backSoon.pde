// Present a "Will be back soon web page", as stand-in webserver.
// jcw, 2011-01-30
 
#include <EtherCard.h>

// ethernet mac address - must be unique on your network
static byte mymac[] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

byte gPacketBuffer[1000]; // tcp/ip send and receive buffer
static BufferFiller bfill;

EtherCard eth (sizeof gPacketBuffer);
DHCPinfo dhcp;

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
    eth.spiInit();
    eth.dhcpInit(mymac, dhcp);
    while (!eth.dhcpCheck(eth.packetReceive()))
        ;
    eth.printIP("IP: ", dhcp.myip);
    eth.initIp(mymac, dhcp.myip, 80); // HTTP port
}

void loop(){
    word len = eth.packetReceive();
    word pos = eth.packetLoop(len);
    
    if (pos) {
        bfill = eth.tcpOffset();
        bfill.emit_p(page);
        eth.httpServerReply(bfill.position());
    }
}
