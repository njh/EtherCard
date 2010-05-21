// DNS look-up functions based on the udp client
// Author: Guido Socher 
// Copyright: GPL V2
//
// Mods bij jcw, 2010-05-20

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdlib.h>
#include "net.h"
#include "ip_arp_udp_tcp.h"
#include "ip_config.h"
#include "WConstants.h"  //Arduino

static byte dnstid_l=0; // a counter for transaction ID
#define DNSCLIENT_SRC_PORT_H 0xe0 
static byte dnsip[]={8,8,8,8}; // the google public DNS, don't change unless there is a real need
static byte haveDNSanswer=0;
static byte dns_answerip[4];
static byte dns_ansError=0;

byte dnslkup_haveanswer(void) {   
    return(haveDNSanswer);
}

byte dnslkup_get_error_info(void) {   
    return(dns_ansError);
}

byte *dnslkup_getip(void) {   
    return(dns_answerip);
}

void dnslkup_request(byte *buf,const prog_char *progmem_hostname) {
    byte i,lenpos,lencnt;
    char c;
    haveDNSanswer=0;
    dns_ansError=0;
    dnstid_l++; // increment for next request, finally wrap
    send_udp_prepare(buf,(DNSCLIENT_SRC_PORT_H<<8)|(dnstid_l&0xff),dnsip,53);
    buf[UDP_DATA_P+1]=dnstid_l;
    buf[UDP_DATA_P+2]=1; // flags, standard recursive query
    i=3;
    while(i<10)
        buf[UDP_DATA_P+i++]=0;
    buf[UDP_DATA_P+5]=1; // 1 question
    lenpos=12;
    i=13;
    lencnt=0;
    while ((c = pgm_read_byte(progmem_hostname++))) {
        if (c=='.'){
            buf[UDP_DATA_P+lenpos]=lencnt;
            lencnt=0;
            lenpos=i;
        }
        buf[UDP_DATA_P+i]=c;
        lencnt++;
        i++;
    }
    buf[UDP_DATA_P+lenpos]=lencnt-1;
    buf[UDP_DATA_P+i++]=0; // terminate with zero, means root domain.
    buf[UDP_DATA_P+i++]=0;
    buf[UDP_DATA_P+i++]=1; // type A
    buf[UDP_DATA_P+i++]=0; 
    buf[UDP_DATA_P+i++]=1; // class IN
    buf[UDP_DATA_P]=i-12;
    send_udp_transmit(buf,i);
}

byte udp_client_check_for_dns_answer(byte *buf,uint16_t plen){
    byte j,i;
    if (plen<70){
        return(0);
    }
    if (buf[UDP_SRC_PORT_L_P]!=53)
        return(0);
    if (buf[UDP_DST_PORT_H_P]!=DNSCLIENT_SRC_PORT_H)
        return(0);
    if (buf[UDP_DST_PORT_L_P]!=dnstid_l)
        return(0);
    if (buf[UDP_DATA_P+1]!=dnstid_l)
        return(0);
    if ((buf[UDP_DATA_P+3]&0x8F)!=0x80){ 
        dns_ansError=1;
        return(0);
    }
    i=12+buf[UDP_DATA_P]; // we encoded the query len into tid
    if (buf[UDP_DATA_P+i] & 0xc0){
        i+=2;
    }else{
        while(i<plen-UDP_DATA_P-7){
            i++;
            if (buf[UDP_DATA_P+i]==0){
                i++;
                break;
            }
        }
    }
    if (buf[UDP_DATA_P+i+9] !=4 ){
        dns_ansError=2; // not IPv4
        return(0);
    }
    i+=10;
    for (j = 0; j < 4; ++j)
        dns_answerip[j]=buf[UDP_DATA_P+i+j];
    haveDNSanswer=1;
    return(1);
}
