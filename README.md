# EtherCard

**EtherCard** is a driver for the ENC28J60 chip, compatible with Arduino IDE.  
Adapted and extended from code written by Guido Socher and Pascal Stang.

License: GPL2

The documentation for this library is at http://jeelabs.net/pub/docs/ethercard/.

## Library Installation

1. Download the ZIP file from https://github.com/jcw/ethercard/archive/master.zip
2. From the Arduino IDE: Sketch -> Import Library... -> Add Library...
3. Restart the Arduino IDE to see the new "EtherCard" library with examples

See the comments in the example sketches for details about how to try them out.

## Physical Installation

PIN Connections (Using Arduino UNO):

    VCC -   3.3V
    GND -    GND
    SCK - Pin 13
    SO  - Pin 12
    SI  - Pin 11
    CS  - Pin  8 # Selectable with the ether.begin() function

## Support

For questions and help, see the [forums][F] (at JeeLabs.net).  
The issue tracker has been moved back to [Github][I] again.

[F]: http://jeenet.net/projects/cafe/boards
[I]: https://github.com/jcw/ethercard/issues
