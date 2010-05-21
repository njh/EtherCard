// This file determines which functionallity of the TCP/IP stack is included.
// Author: Guido Socher 
// Copyright: GPL V2
//
// The TCP implementation uses some size optimisations which are valid
// only if all data can be sent in one single packet. This is however
// not a big limitation for a microcontroller as you will anyhow use
// small web-pages. The web server must send the entire web page in one
// packet. The client "web browser" as implemented here can also receive
// large pages.
//
// Mods bij jcw, 2010-05-20

#ifndef IP_CONFIG_H
#define IP_CONFIG_H

//------------- functions in ip_arp_udp_tcp.c --------------
// an NTP client (ntp clock):
#define NTP_client
// a spontanious sending UDP client
#define UDP_client
//

// to send out a ping:
#define PING_client
#define PINGPATTERN 0x42

// a UDP wake on lan sender:
#define WOL_client

// a "web browser". This can be use to upload data
// to a web server on the internet by encoding the data 
// into the url (like a Form action of type GET):
#define WWW_client
// if you do not need a browser and just a server:
//#undef WWW_client

#define TCP_client

//
//------------- functions in websrv_help_functions.c --------------
//
// functions to decode cgi-form data:
#define FROMDECODE_websrv_help

// function to encode a URL (mostly needed for a web client)
#define URLENCODE_websrv_help

#endif /* IP_CONFIG_H */
