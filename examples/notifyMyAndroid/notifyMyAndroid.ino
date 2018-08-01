// This demo shows how to send a notification to the Notify My Android service
//
// Warning: Due to the limitations of the Arduino, this demo uses insecure
// HTTP to interact with the nma api (not HTTPS). The API key WILL be sent
// accross the wire in plain text.
//
// 2015-04-10 <jc@wippler.nl>
//
// License: GPLv2

#include <EtherCard.h>

const char apihost[] PROGMEM = "www.notifymyandroid.com";

static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

byte Ethernet::buffer[900];
Stash stash;
static byte session;

static void notifyMyAndroid () {
  byte sd = stash.create();

  stash.print("apikey=");
  stash.print("ADD YOUR API KEY HERE");

  stash.print("&application=");
  stash.print("arduino");

  stash.print("&event=");
  stash.print("Ethercard Notify My Android Example");

  stash.print("&description=");
  stash.print("Test message from an Arduino!");

  stash.print("&priority=");
  stash.print("0");

  stash.save();
  int stash_size = stash.size();

  // Compose the http POST request, taking the headers below and appending
  // previously created stash in the sd holder.
  Stash::prepare(PSTR("POST /publicapi/notify HTTP/1.1" "\r\n"
                      "Host: $F" "\r\n"
                      "Content-Length: $D" "\r\n"
                      "Content-Type: application/x-www-form-urlencoded" "\r\n"
                      "\r\n"
                      "$H"),
                 apihost, stash_size, sd);

  // send the packet - this also releases all stash buffers once done
  // Save the session ID so we can watch for it in the main loop.
  session = ether.tcpSend();
}

void setup () {
  Serial.begin(57600);
  Serial.println("\nStarting Notify My Android Example");

  // Change 'SS' to your Slave Select pin, if you arn't using the default pin
  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

  if (!ether.dnsLookup(apihost))
    Serial.println(F("DNS lookup failed for the apihost"));
  ether.printIp("SRV: ", ether.hisip);

  notifyMyAndroid();
}

void loop () {
  ether.packetLoop(ether.packetReceive());

  const char* reply = ether.tcpReply(session);
  if (reply != 0) {
    Serial.println("Got a response!");
    Serial.println(reply);
  }
}
