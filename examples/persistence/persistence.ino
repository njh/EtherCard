// Demo for using persistence flag and readPacketSlice()
//
// License: GPLv2

#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

// the buffersize must be relatively large for DHCP to work; when
// using static setup a buffer size of 100 is sufficient;
#define BUFFERSIZE 350
byte Ethernet::buffer[BUFFERSIZE];

const char website[] PROGMEM = "textfiles.com";

uint32_t nextSeq;
// called when the client request is complete
static void my_callback (byte status, word off, word len) {

    if (strncmp_P((char*) Ethernet::buffer+off,PSTR("HTTP"),4) == 0) {
        Serial.println(">>>");
        // first reply packet
        nextSeq = ether.getSequenceNumber();
    }

    if (nextSeq != ether.getSequenceNumber()) { Serial.print(F("<IGNORE DUPLICATE(?) PACKET>")); return; }

    uint16_t payloadlength = ether.getTcpPayloadLength();
    int16_t chunk_size   = BUFFERSIZE-off-1;
    int16_t char_written = 0;
    int16_t char_in_buf  = chunk_size < payloadlength ? chunk_size : payloadlength;

    while (char_written < payloadlength) {
        Serial.write((char*) Ethernet::buffer+off,char_in_buf);
        char_written += char_in_buf;

        char_in_buf = ether.readPacketSlice((char*) Ethernet::buffer+off,chunk_size,off+char_written);
    }

    nextSeq += ether.getTcpPayloadLength();
}

void setup () {
    Serial.begin(57600);
    Serial.println(F("\n[Persistence+readPacketSlice]"));

    // Change 'SS' to your Slave Select pin, if you arn't using the default pin
    if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
        Serial.println(F("Failed to access Ethernet controller"));
    if (!ether.dhcpSetup())
        Serial.println(F("DHCP failed"));

    ether.printIp("IP:  ", ether.myip);
    ether.printIp("GW:  ", ether.gwip);
    ether.printIp("DNS: ", ether.dnsip);

    if (!ether.dnsLookup(website))
        Serial.println("DNS failed");

    ether.printIp("SRV: ", ether.hisip);
    ether.persistTcpConnection(true);
}

void loop () {
  ether.packetLoop(ether.packetReceive());

  static uint32_t timer =  0;
  if (millis() > timer) {
      timer = millis() + 7000;
      Serial.println();
      Serial.print("<<< REQ ");
      ether.browseUrl(PSTR("/stories/"), "cmoutmou.txt", website, my_callback);
  }
}
