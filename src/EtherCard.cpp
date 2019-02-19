// This code slightly follows the conventions of, but is not derived from:
//      EHTERSHIELD_H library for Arduino etherShield
//      Copyright (c) 2008 Xing Yu.  All right reserved. (this is LGPL v2.1)
// It is however derived from the enc28j60 and ip code (which is GPL v2)
//      Author: Pascal Stang
//      Modified by: Guido Socher
//      DHCP code: Andrew Lindsay
// Hence: GPL V2
//
// 2010-05-19 <jc@wippler.nl>

#include "EtherCard.h"
#include <stdarg.h>
#include <avr/eeprom.h>

EtherCard ether;

uint8_t EtherCard::mymac[ETH_LEN];  // my MAC address
uint8_t EtherCard::myip[IP_LEN];   // my ip address
uint8_t EtherCard::netmask[IP_LEN]; // subnet mask
uint8_t EtherCard::broadcastip[IP_LEN]; // broadcast address
uint8_t EtherCard::gwip[IP_LEN];   // gateway
uint8_t EtherCard::dhcpip[IP_LEN]; // dhcp server
uint8_t EtherCard::dnsip[IP_LEN];  // dns server
uint8_t EtherCard::hisip[IP_LEN];  // ip address of remote host
uint16_t EtherCard::hisport = HTTP_PORT; // tcp port to browse to
bool EtherCard::using_dhcp = false;
bool EtherCard::persist_tcp_connection = false;
uint16_t EtherCard::delaycnt = 0; //request gateway ARP lookup

uint8_t EtherCard::begin (const uint16_t size,
                          const uint8_t* macaddr,
                          uint8_t csPin) {
    using_dhcp = false;
#if ETHERCARD_STASH
    Stash::initMap();
#endif
    copyMac(mymac, macaddr);
    return initialize(size, mymac, csPin);
}

bool EtherCard::staticSetup (const uint8_t* my_ip,
                             const uint8_t* gw_ip,
                             const uint8_t* dns_ip,
                             const uint8_t* mask) {
    using_dhcp = false;

    if (my_ip != 0)
        copyIp(myip, my_ip);
    if (gw_ip != 0)
        setGwIp(gw_ip);
    if (dns_ip != 0)
        copyIp(dnsip, dns_ip);
    if(mask != 0)
        copyIp(netmask, mask);
    updateBroadcastAddress();
    delaycnt = 0; //request gateway ARP lookup
    return true;
}

char* EtherCard::wtoa(uint16_t value, char* ptr)
{
    if (value > 9)
        ptr = wtoa(value / 10, ptr);
    *ptr = '0' + value % 10;
    *++ptr = 0;
    return ptr;
}
