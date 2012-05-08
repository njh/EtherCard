// Twitter client sketch for ENC28J60 based Ethernet Shield. Uses supertweet.net
// as an OAuth authentication proxy. Step by step instructions:
// 
//  1. Create an account on www.supertweet.net by logging with your twitter
//     credentials.
//  2. You'll be redirected to twitter to allow supertweet to post for you
//     on twitter.
//  3. Back on supertweet, set a password (different than your twitter one) and
//     activate your account clicking on "Make Active" link in from the table.
//  4. Wait for supertweet email to confirm your email address - won't work
//     otherwise.
//  5. Encode "un:pw" in base64: "un" being your twitter username and "pw" the
//     password you just set for supertweet.net. You can use this tool:
//        http://tuxgraphics.org/~guido/javascript/base64-javascript.html
//  6. Paste the result as the KEY string in the code bellow.
//
// Contributed by Inouk Bourgon <contact@inouk.imap.cc>
//     http://opensource.org/licenses/mit-license.php
 
#include <EtherCard.h>

// supertweet.net username:password in base64
#define KEY   "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

char website[] PROGMEM = "api.supertweet.net";

byte Ethernet::buffer[700];
Stash stash;
uint8_t runTime;

static void sendToTwitter () {
	// generate two fake values as payload - by using a separate stash,
	// we can determine the size of the generated message ahead of time
	byte sd = stash.create();
	stash.println("status=@inoukb my #Arduino tweets :-)");
	stash.save();
	  
	// generate the header with payload - note that the stash size is used,
	// and that a "stash descriptor" is passed in as argument using "$H"
	Stash::prepare(PSTR("POST /1/statuses/update.xml HTTP/1.1" "\r\n"
          						"Host: $F" "\r\n"
          						"Authorization: Basic $F" "\r\n"
          						"User-Agent: Arduino EtherCard lib" "\r\n"                        
          						"Content-Length: $D" "\r\n"
          						"Content-Type: application/x-www-form-urlencoded" "\r\n"
          						"\r\n"
          						"$H"),
					        website, PSTR(KEY), stash.size(), sd);
	
	// send the packet - this also releases all stash buffers once done
	ether.tcpSend();
}

void setup () {
  Serial.begin(57600);
  Serial.println("\n[webClient]");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
    
  ether.printIp("SRV: ", ether.hisip);

  sendToTwitter();
}

void loop () {
  ether.packetLoop(ether.packetReceive());
}
