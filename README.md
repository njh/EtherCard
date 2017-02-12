# EtherCard

[![Travis Status][S]][T]

**EtherCard** is a driver for the ENC28J60 chip, compatible with Arduino IDE.
Adapted and extended from code written by Guido Socher and Pascal Stang.

License: GPL2

The documentation for this library is at http://jeelabs.net/pub/docs/ethercard/.

## Requirements

* Hardware: This library **only** supports the ENC28J60 chip.
* Hardware: Only AVR-based microcontrollers are supported, such as:
    * Arduino Uno
    * Arduino Mega
    * Arduino Leonardo
    * Arduino Nano/Pro/Fio/Pro-mini/LiliPad/Duemilanove
    * Any other Arduino clone using an AVR microcontroller should work
* Hardware: Non-AVR boards are **NOT** currently supported (101/Zero/Due)
  [#211](https://github.com/jcw/ethercard/issues/211#issuecomment-255011491)
* Hardware: Depending on the size of the buffer for packets, this library
  uses about 1k of Arduino RAM. Large strings and other global variables
  can easily push the limits of smaller microcontrollers.
* Hardware: This library uses the SPI interface of the microcontroller,
  and will require at least one dedicated pin for CS, plus the SO, SI, and
  SCK pins of the SPI interface.
* Software: Any Arduino IDE >= 1.0.0 should be fine

## Library Installation

1. Download the ZIP file from https://github.com/jcw/ethercard/archive/master.zip
2. Rename the downloaded file to `ethercard.zip`
3. From the Arduino IDE: Sketch -> Include Library... -> Add .ZIP Library...
4. Restart the Arduino IDE to see the new "EtherCard" library with examples

See the comments in the example sketches for details about how to try them out.

## Physical Installation

### PIN Connections (Using Arduino UNO):

    VCC -   3.3V
    GND -    GND
    SCK - Pin 13
    SO  - Pin 12
    SI  - Pin 11
    CS  - Pin 10 # Selectable with the ether.begin() function

### PIN Connections using an Arduino Mega

    VCC -   3.3V
    GND -    GND
    SCK - Pin 52
    SO  - Pin 50
    SI  - Pin 51
    CS  - Pin 53 # Selectable with the ether.begin() function

## Support

For questions and help, see the [forums][F] (at JeeLabs.net).
The issue tracker has been moved back to [Github][I] again.

[F]: http://jeelabs.net/projects/cafe/boards
[I]: https://github.com/jcw/ethercard/issues
[S]: https://travis-ci.org/jcw/ethercard.svg
[T]: https://travis-ci.org/jcw/ethercard

## Related Work

There are other Arduino libraries for the ENC28J60 that are worth mentioning:

* [UIPEthernet](https://github.com/ntruchsess/arduino_uip) (Drop in replacement for stock Arduino Ethernet library)
* [EtherSia](https://github.com/njh/EtherSia) (IPv6 Arduino library for ENC28J60)
* [EtherShield](https://github.com/thiseldo/EtherShield) (no longer maintained, predecessor to Ethercard)
* [ETHER_28J60](https://github.com/muanis/arduino-projects/tree/master/libraries/ETHER_28J60) (no longer maintained, very low footprint and simple)

Read more about the differences at [this blog post](http://www.tweaking4all.com/hardware/arduino/arduino-enc28j60-ethernet/).
