/* 
 * ds18b20_pachube.ino
 *
 * Copyright (C) 2012 Jonathan McCrohan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Originally based on code by Jean-Claude Wippler <jc@wippler.nl>
 *
 * Requires three libraries:
 * Ethercard - https://github.com/jcw/ethercard
 * OneWire - http://www.pjrc.com/teensy/td_libs_OneWire.html
 * DallasTemperature - http://code.google.com/p/dallas-temperature-control-library/
 */

#include <EtherCard.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 18

// change these settings to match your own setup
#define FEED    "14613"
#define APIKEY  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ethernet interface mac address, unique on LAN
byte mymac[] = { 
  0x54,0x55,0x58,0x10,0x00,0x25 };

char website[] PROGMEM = "api.pachube.com";

byte Ethernet::buffer[700];
BufferFiller bfill;
uint32_t timer;
Stash stash;

static char statusstr[10];

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup () {

  // use pins 19 and 17 to provide power for DS18B20
  digitalWrite(19, LOW);
  digitalWrite(17, HIGH);
  pinMode(19, OUTPUT);
  pinMode(17, OUTPUT);
  sensors.begin();

  Serial.begin(57600);
  Serial.println("\n[webClient]");

  int chipselectpin = 10;

  if (ether.begin(sizeof Ethernet::buffer, mymac, chipselectpin) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");

  ether.printIp("SRV: ", ether.hisip);

  Serial.println();
  Serial.print("DS18B20 POV: ");
  sensors.requestTemperatures();
  // should read 85.00C as standard power on value.
  Serial.println(sensors.getTempCByIndex(0));
  delay(5000);

  Serial.print("DS18B20 Temp: ");
  sensors.requestTemperatures(); 
  Serial.println(sensors.getTempCByIndex(0));
}

// Simple web page for anyone using a web browser
static word homePage() {
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
  "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "<pre>"
    "$S"
    " &deg;C</pre>"), statusstr);
  return bfill.position();
}


void loop () {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  // handy web requests
  if (pos)
    ether.httpServerReply(homePage());

  if (millis() > timer) {
    timer = millis() + 30000;

    sensors.requestTemperatures();
    dtostrf(sensors.getTempCByIndex(0),4, 4,statusstr);
    byte sd = stash.create();
    stash.print("0,");
    stash.println(statusstr);
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
