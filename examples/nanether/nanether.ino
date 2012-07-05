// Example of EtherCard usage, contributed by Will Rose, 2012-07-05.

#include <EtherCard.h>

#define BUF_SIZE 512

byte mac[] = { 0x00, 0x04, 0xA3, 0x21, 0xCA, 0x38 };   // Nanode MAC address.

uint8_t ip[] = { 192, 168, 1, 8 };          // The fallback board address.
uint8_t dns[] = { 192, 168, 1, 20 };        // The DNS server address.
uint8_t gateway[] = { 192, 168, 1, 20 };    // The gateway router address.
uint8_t subnet[] = { 255, 255, 255, 0 };    // The subnet mask.
byte Ethernet::buffer[BUF_SIZE];
byte fixed;                                 // Address fixed, no DHCP
const char * DHCPstates[] = {
    "DHCP_STATE_BAD",
    "DHCP_STATE_INIT",
    "DHCP_STATE_SELECT",
    "DHCP_STATE_REQUEST",
    "DHCP_STATE_BOUND",
    "DHCP_STATE_RENEW",
    "DHCP_STATE_REBIND"
};

void setup(void)
{
    Serial.begin(57600);
    delay(2000);

    /* Check that the Ethernet controller exists */
    Serial.println("Initialising the Ethernet controller");
    if (ether.begin(sizeof Ethernet::buffer, mac, 8) == 0) {
        Serial.println( "Ethernet controller NOT initialised");
        while (true)
            /* MT */ ;
    }

    /* Get a DHCP connection */
    Serial.println("Attempting to get an IP address using DHCP");
    fixed = false;
    if (ether.dhcpSetup("Nanode")) {
        ether.printIp("Got an IP address using DHCP: ", ether.myip);
        Serial.print("DHCP lease time ");
        Serial.print(ether.dhcpLeaseTime(), DEC);
        Serial.println(" msec");
    }
    /* If DHCP fails, start with a hard-coded address */
    else {
        ether.staticSetup(ip, gateway, dns);
        ether.printIp("DHCP FAILED, using fixed address: ", ether.myip);
        fixed = true;
    }

    return;
}

void loop(void)
{
    word rc;

    Serial.print("DHCP state : ");
    Serial.println(DHCPstates[ether.dhcpFSM()]);

    /* Check the DHCP lease timers, and update if necessary */
    ether.dhcpLease();
    if (!ether.dhcpValid()) {
        /* Apparently the EN28J60 can get confused enough
        ** to require a reset.
        */
        if (ether.begin(sizeof Ethernet::buffer, mac, 8) == 0)
            Serial.println( "Ethernet controller NOT initialised");
        else {
            Serial.println( "Ethernet controller initialised");
            for (int j = 0 ; j < 1 ; j++) {
                if (ether.dhcpSetup("Nanode")) {
                    Serial.println("dhcpSetup() returned TRUE");
                    break;
                }
                else
                    Serial.println("dhcpSetup() returned FALSE");
                /* Let things settle */
                delay(1000);
            }
        }
    }

    /* These function calls are required if ICMP packets are to be accepted */
    rc = ether.packetLoop(ether.packetReceive());
    Serial.print("ether.packetLoop() returned ");
    Serial.println(rc, DEC);

    // For debugging
    delay(5000);

    return;
}
