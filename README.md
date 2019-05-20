# EtherCard

[![Travis Status][S]][T]

**EtherCard** is a driver for the Microchip ENC28J60 chip, compatible with Arduino IDE.
It is adapted and extended from code written by Guido Socher and Pascal Stang.

High-level routines are provided to allow a variety of purposes 
including simple data transfer through to HTTP handling.

License: GPL2


## Requirements

* Hardware: This library **only** supports the ENC28J60 chip.
* Hardware: Only AVR-based microcontrollers are supported, such as:
    * Arduino Uno
    * Arduino Mega
    * Arduino Leonardo
    * Arduino Nano/Pro/Fio/Pro-mini/LiliPad/Duemilanove
    * Any other Arduino clone using an AVR microcontroller should work
* Hardware: Non-AVR boards are **NOT** currently supported (101/Zero/Due)
  [#211](https://github.com/njh/EtherCard/issues/211#issuecomment-255011491)
* Hardware: Depending on the size of the buffer for packets, this library
  uses about 1k of Arduino RAM. Large strings and other global variables
  can easily push the limits of smaller microcontrollers.
* Hardware: This library uses the SPI interface of the microcontroller,
  and will require at least one dedicated pin for CS, plus the SO, SI, and
  SCK pins of the SPI interface. An interrupt pin is not required.
* Software: Any Arduino IDE >= 1.0.0 should be fine


## Library Installation

EtherCard is available for installation in the [Arduino Library Manager].
Alternatively it can be downloaded directly from GitHub:

1. Download the ZIP file from https://github.com/njh/EtherCard/archive/master.zip
2. Rename the downloaded file to `ethercard.zip`
3. From the Arduino IDE: Sketch -> Include Library... -> Add .ZIP Library...
4. Restart the Arduino IDE to see the new "EtherCard" library with examples

See the comments in the example sketches for details about how to try them out.


## Physical Installation

### PIN Connections (Using Arduino UNO or Arduino NANO):

| ENC28J60 | Arduino Uno | Notes                                       |
|----------|-------------|---------------------------------------------|
| VCC      | 3.3V        |                                             |
| GND      | GND         |                                             |
| SCK      | Pin 13      |                                             |
| MISO     | Pin 12      |                                             |
| MOSI     | Pin 11      |                                             |
| CS       | Pin 10      | Selectable with the ether.begin() function  |


### PIN Connections using an Arduino Mega

| ENC28J60 | Arduino Mega | Notes                                       |
|----------|--------------|---------------------------------------------|
| VCC      | 3.3V         |                                             |
| GND      | GND          |                                             |
| SCK      | Pin 52       |                                             |
| MISO     | Pin 50       |                                             |
| MOSI     | Pin 51       |                                             |
| CS       | Pin 53       | Selectable with the ether.begin() function  |


## Using the library

Full API documentation for this library is at: http://www.aelius.com/njh/ethercard/

Several [example sketches] are provided with the library which demonstrate various features.
Below are descriptions on how to use the library.

Note: `ether` is a globally defined instance of the `EtherCard` class and may be used to access the library.


### Initialising the library

Initiate To initiate the library call `ether.begin()`.

```cpp
uint8_t Ethernet::buffer[700]; // configure buffer size to 700 octets
static uint8_t mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 }; // define (unique on LAN) hardware (MAC) address

uint8_type vers = ether.begin(sizeof Ethernet::buffer, mymac);
if(vers == 0)
{
    // handle failure to initiate network interface
}
```

### Configure using DHCP

To configure IP address via DHCP use `ether.dhcpSetup()`.

```cpp
if(!ether.dhcpSetup())
{
    // handle failure to obtain IP address via DHCP
}
ether.printIp("IP:   ", ether.myip); // output IP address to Serial
ether.printIp("GW:   ", ether.gwip); // output gateway address to Serial
ether.printIp("Mask: ", ether.netmask); // output netmask to Serial
ether.printIp("DHCP server: ", ether.dhcpip); // output IP address of the DHCP server
```

### Static IP Address

To configure a static IP address use `ether.staticSetup()`.

```cpp
const static uint8_t ip[] = {192,168,0,100};
const static uint8_t gw[] = {192,168,0,254};
const static uint8_t dns[] = {192,168,0,1};

if(!ether.staticSetup(ip, gw, dns);
{
    // handle failure to configure static IP address (current implementation always returns true!)
}
```

### Send UDP packet

To send a UDP packet use `ether.sendUdp()`.

```C
char payload[] = "My UDP message";
uint8_t nSourcePort = 1234;
uint8_t nDestinationPort = 5678;
uint8_t ipDestinationAddress[IP_LEN];
ether.parseIp(ipDestinationAddress, "192.168.0.200");

ether.sendUdp(payload, sizeof(payload), nSourcePort, ipDestinationAddress, nDestinationPort);
```

### DNS Lookup

To perform a DNS lookup use `ether.dnsLookup()`.

```
if(!ether.dnsLookup("google.com"))
{
    // handle failure of DNS lookup
}
ether.printIp("Server: ", ether.hisip); // Result of DNS lookup is placed in the hisip member of EtherCard.
```


## Related Work

There are other Arduino libraries for the ENC28J60 that are worth mentioning:

* [UIPEthernet](https://github.com/ntruchsess/arduino_uip) (Drop in replacement for stock Arduino Ethernet library)
* [EtherSia](https://github.com/njh/EtherSia) (IPv6 Arduino library for ENC28J60)
* [EtherShield](https://github.com/thiseldo/EtherShield) (no longer maintained, predecessor to Ethercard)
* [ETHER_28J60](https://github.com/muanis/arduino-projects/tree/master/libraries/ETHER_28J60) (no longer maintained, very low footprint and simple)

Read more about the differences at [this blog post](http://www.tweaking4all.com/hardware/arduino/arduino-enc28j60-ethernet/).


[Arduino Library Manager]: https://www.arduino.cc/en/Guide/Libraries
[example sketches]: https://github.com/njh/EtherCard/tree/master/examples
[S]: https://travis-ci.org/njh/EtherCard.svg
[T]: https://travis-ci.org/njh/EtherCard
