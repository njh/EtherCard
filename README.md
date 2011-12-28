Work in progress - see <http://forum.jeelabs.net/node/457>

Derived from code by Guido Socher and Pascal Stang, hence under GPL2 license.

The home page for this library is <http://jeelabs.org/ethercard>

You can download this project in either
[zip](https://github.com/jcw/ethercard/zipball/master) or
[tar](https://github.com/jcw/ethercard/tarball/master) formats.

Unpack the archive and rename the result to `EtherCard`.  
Put it in the `libraries` folder in your Arduino sketches area.  
Restart the Arduino IDE to make sure it sees the new files.

Updates by Andrew Lindsay, December 2011:

1. Add alternative begin with 3rd parameter to specify CS pin to use. Only 8, 9 and 10 are usable.
Default without 3rd parameter is pin 8.

2. Updated DHCP to allow broadcast packets to be received as some DHCP servers (Some Linux and 
Windows DHCP servers) send responses to mac address ff:ff:ff:ff:ff:ff which would otherwise be
filtered out. At end of DHCP process the broadcast filter is returned to block broadcast packets.
ARP broadcasts are not affected as they have their own filter.

3. Cleared up a number of Arduino 1.0 IDE warnings.

4. Experimental - commented out line 609 of tcpip.cpp to no longer terminate client connections
after 1 packet has been received. This is to allow larger packets and streamed data to be received. 
However, this may not terminate connections correctly. It needs more work to decide if other host
is wanting to close connection. To remove this feature, remove the comment characters // on line 609.


 
