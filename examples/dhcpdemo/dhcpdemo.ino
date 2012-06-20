
// Simple demo for dhcp
// Chris van den Hooven

#include <Arduino.h>
#include <EtherCard.h>

#define LEDPIN  6
#define POLLTIMER    30000
#define BLINKTIMER     100


byte Ethernet::buffer[700];
uint32_t polltimer = 0 , 
         blinktimer = 0 ;
uint32_t NtpTime = 0;


// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };


char page[] PROGMEM =         "HTTP/1.0 200 OK\r\n"
                              "Content-Type: text/html\r\n"
                              "Retry-After: 600\r\n"
                              "\r\n"
                              "<html>"
                                "<head><title>"
                                  "Nanode is alive"
                                "</title></head>"
                                "<body>"
                                  "<h3>Nanode is alive</h3>"
                                "</body>"
                              "</html>";



void BlinkLed() {
  static byte LedStatus = LOW;
  
  LedStatus = (LedStatus == LOW ? HIGH : LOW);
  digitalWrite(LEDPIN, LedStatus);
}


void panic(char msg[]) {
  Serial.println(msg);
  
  while (true) {
    BlinkLed();
    delay(1000);    
  }
}



void setup () {
  Serial.begin(57600);
  Serial.println("\n[StartSetup]");
  pinMode(LEDPIN, OUTPUT);     

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    panic( "Failed to access Ethernet controller");
    
  if (!ether.dhcpSetup())
    panic("DHCP failed");

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  Serial.println("\n[EndSetup]");
}

void loop () {
	
  // check if anything has come in via ethernet
  word len = ether.packetReceive();
  
  // handle DHCP-renew
  ether.DhcpStateMachine(len);
  
  // check if valid tcp data is received (called the webserver)
  word pos = ether.packetLoop(len);  
  if (pos) {
    char* data = (char *) Ethernet::buffer + pos;
    memcpy_P(ether.tcpOffset(), page, sizeof page);
    ether.httpServerReply(sizeof page - 1);
  }
  
  if (millis() > blinktimer) {
    blinktimer = millis() + BLINKTIMER;
    BlinkLed(); 
  }
}
