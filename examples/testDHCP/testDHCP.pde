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

static byte buf[700];
static DHCPinfo dhcp;
static EtherCard es;

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
    
    es.spiInit();

    Serial.println("Setting up DHCP");
    if (es.dhcpInit(mymac, dhcp) == 0) 
        Serial.println( "Failed to access ENC28J60");
}

void loop(){
    word len = es.packetReceive(buf, sizeof buf);
    
    if (es.dhcpCheck(buf, len)) {
        es.printIP("My IP: ", dhcp.myip);
        es.printIP("Netmask: ", dhcp.mymask);
        es.printIP("DNS IP: ", dhcp.dnsip);
        es.printIP("GW IP: ", dhcp.gwip);

        while (1)
            digitalWrite(DHCP_LED, HIGH);
    }
}
