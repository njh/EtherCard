// Arduino demo sketch for testing the DHCP client code
//
// Original author: Andrew Lindsay
// Major rewrite and API overhaul by jcw, 2011-06-07
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#include <EtherCard.h>

#define DHCP_LED 9

static byte mymac[6] = { 0x54,0x55,0x58,0x12,0x34,0x56 };

static byte buf[700];
static DHCPinfo dhcp;
static EtherCard es;

static void printIP (const char* msg, byte *buf) {
    Serial.print(msg);
    for (byte i = 0; i < 4; ++i) {
        Serial.print( buf[i], DEC );
        if (i < 3)
            Serial.print('.');
    }
    Serial.println();
}

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
    if (es.initialize(mymac) == 0) 
        Serial.println( "Failed to access ENC28J60");
            
    Serial.println("Setting up DHCP");
    es.dhcpInit(mymac, dhcp);
}

void loop(){
    word len = es.packetReceive(buf, sizeof buf - 1);
    
    if (es.dhcpCheck(buf, len)) {
        printIP("My IP: ", dhcp.myip);
        printIP("Netmask: ", dhcp.mymask);
        printIP("DNS IP: ", dhcp.dnsip);
        printIP("GW IP: ", dhcp.gwip);

        while (1)
            digitalWrite(DHCP_LED, HIGH);
    }
}
