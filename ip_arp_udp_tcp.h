// IP/ARP/UDP/TCP functions
// Author: Guido Socher 
// Copyright: GPL V2
//
// Mods bij jcw, 2010-05-20

#ifndef IP_ARP_UDP_TCP_H
#define IP_ARP_UDP_TCP_H
#include "ip_config.h"
#include <avr/pgmspace.h>

extern void init_ip_arp_udp_tcp(uint8_t*,uint8_t*,uint16_t);
// extern uint8_t eth_type_is_ip_and_my_ip(uint8_t*,uint16_t);
extern void make_udp_reply_from_request(uint8_t*,char*,uint8_t,uint16_t);
extern uint16_t packetloop_icmp_tcp(uint8_t*,uint16_t);
extern void www_server_reply(uint8_t*,uint16_t);
extern void client_set_gwip(uint8_t*);
// extern void client_gw_arp_refresh();
extern void client_arp_whohas(uint8_t*,uint8_t*);
extern uint8_t client_waiting_gw(); // 1 no GW mac yet, 0 have a gw mac
#define client_set_wwwip client_tcp_set_serverip
extern void client_tcp_set_serverip(uint8_t*);
extern uint8_t client_tcp_req(uint8_t (*)(uint8_t,uint8_t,uint16_t, uint16_t),uint16_t (*)(uint8_t),uint16_t);
extern void client_browse_url(prog_char*, char*, prog_char*,void (*)(uint8_t,uint16_t,uint16_t));
extern void client_http_post(prog_char*, prog_char*, prog_char*,char*,void (*)(uint8_t,uint16_t,uint16_t));
extern void client_ntp_request(uint8_t*,uint8_t*,uint8_t );
extern uint8_t client_ntp_process_answer(uint8_t*,uint32_t*,uint8_t);
extern void send_udp_prepare(uint8_t*,uint16_t, uint8_t*, uint16_t);
extern void send_udp_transmit(uint8_t*,uint8_t);
extern void send_udp(uint8_t*,char*,uint8_t,uint16_t, uint8_t*, uint16_t);
extern void register_ping_rec_callback(void (*)(uint8_t*));
extern void client_icmp_request(uint8_t*,uint8_t*);
extern uint8_t packetloop_icmp_checkreply(uint8_t*,uint8_t*);
extern void send_wol(uint8_t*,uint8_t*);

class TCPIP {
public:
    
    uint8_t* tcpOffset(uint8_t* buf)
        { return buf + 0x36; }

	void initIp(uint8_t *mymac,uint8_t *myip,uint16_t wwwp){
    	init_ip_arp_udp_tcp(mymac,myip,wwwp);
    }

    // uint8_t checkMyIp(uint8_t *buf,uint16_t len){
    //      return eth_type_is_ip_and_my_ip(buf, len);
    //     }

	void makeUdpReply(uint8_t *buf,char *data,uint8_t len,uint16_t port){
    	make_udp_reply_from_request(buf, data, len, port);
    }

	uint16_t packetLoop(uint8_t *buf,uint16_t plen){
    	return packetloop_icmp_tcp(buf, plen);
    }

	void httpServerReply(uint8_t *buf,uint16_t dlen){
    	www_server_reply(buf,dlen);
    }

	void clientSetGwIp(uint8_t *gwipaddr){
    	client_set_gwip(gwipaddr);
    }

	uint8_t clientWaitingGw(void){
    	return client_waiting_gw();
    }

#define clientSetHttpIp clientSetServerIp
	void clientSetServerIp(uint8_t *ipaddr){
    	client_tcp_set_serverip(ipaddr);
    }

    uint8_t clientTcpReq(uint8_t (*rcb)(uint8_t,uint8_t,uint16_t, uint16_t),uint16_t (*dfcb)(uint8_t),uint16_t port) {
    	return client_tcp_req(rcb, dfcb, port);
    }

	void browseUrl(prog_char *urlbuf, char *urlbuf_varpart, prog_char *hoststr,void (*cb)(uint8_t,uint16_t,uint16_t)){
    	client_browse_url(urlbuf, urlbuf_varpart, hoststr,cb);
    }

	void httpPost(prog_char *urlbuf, prog_char *hoststr, prog_char *header, char *postval,void (*cb)(uint8_t,uint16_t,uint16_t)){
    	client_http_post(urlbuf, hoststr, header, postval,cb);
    }

	void ntpRequest(uint8_t *buf,uint8_t *ntpip,uint8_t srcport){
    	client_ntp_request(buf,ntpip,srcport);
    }

	uint8_t ntpProcessAnswer(uint8_t *buf,uint32_t *time,uint8_t dstport_l){
    	return client_ntp_process_answer(buf,time,dstport_l);
    }

	void udpPrepare(uint8_t *buf,uint16_t sport, uint8_t *dip, uint16_t dport){
    	send_udp_prepare(buf, sport, dip, dport);
    }

	void udpTransmit(uint8_t *buf,uint8_t len){
    	send_udp_transmit(buf, len);
    }

	void sendUdp(uint8_t *buf,char *data,uint8_t len,uint16_t sport, uint8_t *dip, uint16_t dport){
    	send_udp(buf, data, len, sport, dip, dport);
    }

	void registerPingCallback(void (*cb)(uint8_t*)){
    	register_ping_rec_callback(cb);
    }

	void clientIcmpRequest(uint8_t *buf,uint8_t *destip){
    	client_icmp_request(buf,destip);
    }

	uint8_t packetLoopIcmpCheckReply(uint8_t *buf,uint8_t *ip_mh){
    	return packetloop_icmp_checkreply(buf,ip_mh);
    }

	void sendWol(uint8_t *buf,uint8_t *wolmac){
    	send_wol(buf,wolmac);
    }
};

#endif /* IP_ARP_UDP_TCP_H */
