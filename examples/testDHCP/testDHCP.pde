// Arduino demo sketch for testing the DHCP client code
//
// Original author: Andrew Lindsay
// Major rewrite and API overhaul by jcw, 2011-06-07
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include <EtherCard.h>

#define DHCP_LED 9

static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };

byte gPacketBuffer[700];
static EtherCard eth (sizeof gPacketBuffer);
static DHCPinfo dhcp;

void setup(){
    Serial.begin(57600);
    Serial.println("\n[testDHCP]");

    pinMode(DHCP_LED, OUTPUT);
    digitalWrite(DHCP_LED, LOW);
    
    Serial.print("MAC: ");
    for (byte i = 0; i < 6; ++i) {
        Serial.print(mymac[i], HEX);
        if (i < 5)
            Serial.print(':');
    }
    Serial.println();
    
    eth.spiInit();

    Serial.println("Setting up DHCP");
    if (eth.dhcpInit(mymac, dhcp) == 0) 
        Serial.println( "Failed to access Ethernet controller");
}

void loop(){
    word len = eth.packetReceive();
    
    if (eth.dhcpCheck(len)) {
        eth.printIP("My IP: ", dhcp.myip);
        eth.printIP("Netmask: ", dhcp.mymask);
        eth.printIP("DNS IP: ", dhcp.dnsip);
        eth.printIP("GW IP: ", dhcp.gwip);

        while (1)
            digitalWrite(DHCP_LED, HIGH);
    }
}
