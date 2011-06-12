// DNS look-up functions based on the udp client
// Author: Guido Socher 
// Copyright: GPL V2
//
// Mods bij jcw, 2010-05-20

#include "EtherCard.h"
#include "net.h"

#define gPB gPacketBuffer

static byte dnstid_l=0; // a counter for transaction ID
#define DNSCLIENT_SRC_PORT_H 0xe0 
static byte dnsip[]={8,8,8,8}; // the google public DNS, don't change unless there is a real need
static byte haveDNSanswer;
static byte dns_answerip[4];
static byte dns_ansError;

// byte EtherCard::dnsErrorInfo () {   
//     return dns_ansError;
// }

const byte* EtherCard::dnsGetIp () {   
    return dns_answerip;
}

static byte checkForDnsAnswer (uint16_t plen) {
    byte j,i;
    if (plen<70){
        return(0);
    }
    if (gPB[UDP_SRC_PORT_L_P]!=53)
        return(0);
    if (gPB[UDP_DST_PORT_H_P]!=DNSCLIENT_SRC_PORT_H)
        return(0);
    if (gPB[UDP_DST_PORT_L_P]!=dnstid_l)
        return(0);
    if (gPB[UDP_DATA_P+1]!=dnstid_l)
        return(0);
    if ((gPB[UDP_DATA_P+3]&0x8F)!=0x80){ 
        dns_ansError=1;
        return(0);
    }
    i=12+gPB[UDP_DATA_P]; // we encoded the query len into tid
ChecNextResp:
    if (gPB[UDP_DATA_P+i] & 0xc0){
        i+=2;
    }else{
        while(i<plen-UDP_DATA_P-7){
            i++;
            if (gPB[UDP_DATA_P+i]==0){
                i++;
                break;
            }
        }
    }
    if (gPB[UDP_DATA_P+i+1] != 1){    // check type == 1 for "A"
        i += 2 + 2 + 4;    // skip type & class & TTL
        i += gPB[UDP_DATA_P+i+1] + 2;    // skip datalength bytes
        goto ChecNextResp;
    }
    if (gPB[UDP_DATA_P+i+9] !=4 ){
        dns_ansError=2; // not IPv4
        return(0);
    }
    i+=10;
    for (j = 0; j < 4; ++j)
        dns_answerip[j]=gPB[UDP_DATA_P+i+j];
    haveDNSanswer=1;
    return(1);
}

static void dnsRequest (const prog_char *progmem_hostname) {
    byte i,lenpos,lencnt;
    char c;
    haveDNSanswer=0;
    dns_ansError=0;
    dnstid_l++; // increment for next request, finally wrap
    EtherCard::udpPrepare((DNSCLIENT_SRC_PORT_H<<8)|(dnstid_l&0xff),dnsip,53);
    gPB[UDP_DATA_P+1]=dnstid_l;
    gPB[UDP_DATA_P+2]=1; // flags, standard recursive query
    i=3;
    while(i<10)
        gPB[UDP_DATA_P+i++]=0;
    gPB[UDP_DATA_P+5]=1; // 1 question
    lenpos=12;
    i=13;
    lencnt=0;
    while ((c = pgm_read_byte(progmem_hostname++))) {
        if (c=='.'){
            gPB[UDP_DATA_P+lenpos]=lencnt;
            lencnt=0;
            lenpos=i;
        } else {
            gPB[UDP_DATA_P+i]=c;
            lencnt++;
        }
        i++;
    }
    gPB[UDP_DATA_P+lenpos]=lencnt;
    gPB[UDP_DATA_P+i++]=0; // terminate with zero, means root domain.
    gPB[UDP_DATA_P+i++]=0;
    gPB[UDP_DATA_P+i++]=1; // type A
    gPB[UDP_DATA_P+i++]=0; 
    gPB[UDP_DATA_P+i++]=1; // class IN
    gPB[UDP_DATA_P]=i-12;
    EtherCard::udpTransmit(i);
}

// lookup a host name via DNS, returns 1 if ok or 0 if this timed out
// use during setup, as this discards all incoming requests until it returns
byte EtherCard::dnsLookup (prog_char* name) {
    while (EtherCard::clientWaitingGw())
        EtherCard::packetLoop(packetReceive());

    dnsRequest(name);

    uint32_t start = millis();
    while (!haveDNSanswer) {
        if (millis() - start >= 30000)
            return 0;
        word len = packetReceive();
        word pos = packetLoop(len);
        if (len != 0 && pos == 0)
            checkForDnsAnswer(len);
    }

    clientSetServerIp(dnsGetIp());
    return 1;
}
