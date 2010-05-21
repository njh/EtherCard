// DNS look-up functions based on the udp client
// Author: Guido Socher 
// Copyright: GPL V2
//
// Mods bij jcw, 2010-05-20

#ifndef DNSLKUP_H
#define DNSLKUP_H
#include "ip_config.h"

extern void dnslkup_request(uint8_t*,const prog_char*);
extern uint8_t dnslkup_haveanswer();
extern uint8_t dnslkup_get_error_info();
extern uint8_t udp_client_check_for_dns_answer(uint8_t*,uint16_t);
extern uint8_t *dnslkup_getip();

class DNS {
public:
    
	void dnsRequest(uint8_t *buf,const prog_char *progmem_hostname){
    	dnslkup_request(buf, progmem_hostname);
    }

	uint8_t dnsHaveAnswer(void){
    	return dnslkup_haveanswer();
    }

	uint8_t dnsErrorInfo(void){
    	return dnslkup_get_error_info();
    }

	uint8_t checkForDnsAnswer(uint8_t *buf,uint16_t plen){
    	return udp_client_check_for_dns_answer(buf, plen);
    }

	uint8_t *dnsGetIp(void){
    	return dnslkup_getip();
    }
};

#endif /* DNSLKUP_H */
