// IP, Arp, UDP and TCP functions.
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

#include <avr/pgmspace.h>
#include "net.h"
#include "enc28j60.h"
#include "ip_config.h"
#include <WProgram.h>

static byte wwwport_l=80; // server port
static byte wwwport_h;	 // Note: never use same as TCPCLIENT_SRC_PORT_H
static byte tcpclient_src_port_l=1; 
static byte tcp_fd; // a file descriptor, will be encoded into the port
static byte tcpsrvip[4];
static byte tcp_client_state;
static byte tcp_client_port_h;
static byte tcp_client_port_l;
static byte (*client_tcp_result_callback)(byte,byte,uint16_t,uint16_t);
static uint16_t (*client_tcp_datafill_callback)(byte);
#define TCPCLIENT_SRC_PORT_H 11
// #define TCP_client 1
static byte www_fd;
static byte browsertype; // 0 = get, 1 = post
static void (*client_browser_callback)(byte,uint16_t,uint16_t);
static prog_char *client_additionalheaderline;
static char *client_postval;
static prog_char *client_urlbuf;
static char *client_urlbuf_var;
static prog_char *client_hoststr;
static byte *bufptr; // ugly workaround for backward compatibility
static void (*icmp_callback)(byte *ip);
static int16_t delaycnt=1;
static byte gwip[4]; 
static byte gwmacaddr[6];
static byte waitgwmac; // 0=wait, 1=first req no anser, 2=have gwmac, 4=refeshing but have gw mac, 8=accept an arp reply
#define WGW_INITIAL_ARP 1
#define WGW_HAVE_GW_MAC 2
#define WGW_REFRESHING 4
#define WGW_ACCEPT_ARP_REPLY 8
static byte macaddr[6];
static byte ipaddr[4];
static uint16_t info_data_len;
static byte seqnum=0xa; // my initial tcp sequence number

#define CLIENTMSS 550
#define TCP_DATA_START ((uint16_t)TCP_SRC_PORT_H_P+(buf[TCP_HEADER_LEN_P]>>4)*4)

const char arpreqhdr[] PROGMEM = { 0,1,8,0,6,4,0,1 };
const char iphdr[] PROGMEM = { 0x45,0,0,0x82,0,0,0x40,0,0x20 };
const char ntpreqhdr[] PROGMEM = { 0xE3,0,4,0xFA,0,1,0,0,0,1 };

static uint16_t checksum(byte *buf, uint16_t len,byte type){
	uint32_t sum = type==1 ? IP_PROTO_UDP_V+len-8 :
                    type==2 ? IP_PROTO_TCP_V+len-8 : 0;
	while(len >1){
		sum += 0xFFFF & (((uint32_t)*buf<<8)|*(buf+1));
		buf+=2;
		len-=2;
	}
	if (len)
		sum += ((uint32_t)*buf)<<8;
	while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);
	return (uint16_t) sum ^ 0xFFFF;
}

void init_ip_arp_udp_tcp(byte *mymac,byte *myip,uint16_t port){
	wwwport_h=(port>>8);
	wwwport_l=(port);
	byte i=0;
	while(i<4){
		ipaddr[i]=myip[i];
		i++;
	}
	i=0;
	while(i<6){
		macaddr[i]=mymac[i];
		i++;
	}
}

static byte check_ip_message_is_from(byte *buf,byte *ip) {
	byte i=0;
	while(i<4){
		if (buf[IP_SRC_P+i]!=ip[i])
			return 0;
		i++;
	}
	return 1;
}

static byte eth_type_is_arp_and_my_ip(byte *buf,uint16_t len){
	if (len<41)
		return 0;
	if (buf[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V || 
	   buf[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V)
		return 0;
	byte i=0;
	while(i<4){
		if (buf[ETH_ARP_DST_IP_P+i] != ipaddr[i])
			return 0;
		i++;
	}
	return 1;
}

static byte eth_type_is_ip_and_my_ip(byte *buf,uint16_t len){
	if (len<42)
		return 0;
	if (buf[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V || 
	   buf[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V)
		return 0;
	if (buf[IP_HEADER_LEN_VER_P]!=0x45)
		return 0;
	byte i=0;
	while(i<4){
		if (buf[IP_DST_P+i]!=ipaddr[i])
			return 0;
		i++;
	}
	return 1;
}
static void make_eth(byte *buf) {
	byte i=0;
	while(i<6){
		buf[ETH_DST_MAC +i]=buf[ETH_SRC_MAC +i];
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
}
static void fill_ip_hdr_checksum(byte *buf) {
	buf[IP_CHECKSUM_P]=0;
	buf[IP_CHECKSUM_P+1]=0;
	buf[IP_FLAGS_P]=0x40; // don't fragment
	buf[IP_FLAGS_P+1]=0;  // fragement offset
	buf[IP_TTL_P]=64; // ttl
	uint16_t ck=checksum(&buf[IP_P], IP_HEADER_LEN,0);
	buf[IP_CHECKSUM_P]=ck>>8;
	buf[IP_CHECKSUM_P+1]=ck;
}

static void make_ip(byte *buf) {
	byte i=0;
	while(i<4){
		buf[IP_DST_P+i]=buf[IP_SRC_P+i];
		buf[IP_SRC_P+i]=ipaddr[i];
		i++;
	}
	fill_ip_hdr_checksum(buf);
}

static void step_seq(byte *buf,uint16_t rel_ack_num,byte cp_seq) {
	byte i;
	byte tseq;
	i=4;
	while(i>0){
		rel_ack_num=buf[TCP_SEQ_H_P+i-1]+rel_ack_num;
		tseq=buf[TCP_SEQACK_H_P+i-1];
		buf[TCP_SEQACK_H_P+i-1]=0xff&rel_ack_num;
		if (cp_seq)
			buf[TCP_SEQ_H_P+i-1]=tseq;
		else
			buf[TCP_SEQ_H_P+i-1]= 0; // some preset value
		rel_ack_num=rel_ack_num>>8;
		i--;
	}
}

static void make_tcphead(byte *buf,uint16_t rel_ack_num,byte cp_seq) {
	byte i;
	i=buf[TCP_DST_PORT_H_P];
	buf[TCP_DST_PORT_H_P]=buf[TCP_SRC_PORT_H_P];
	buf[TCP_SRC_PORT_H_P]=i;
	i=buf[TCP_DST_PORT_L_P];
	buf[TCP_DST_PORT_L_P]=buf[TCP_SRC_PORT_L_P];
	buf[TCP_SRC_PORT_L_P]=i;
	step_seq(buf,rel_ack_num,cp_seq);
	buf[TCP_CHECKSUM_H_P]=0;
	buf[TCP_CHECKSUM_L_P]=0;
	buf[TCP_HEADER_LEN_P]=0x50;
}

void make_arp_answer_from_request(byte *buf) {
	make_eth(buf);
	buf[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;
	buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
	byte i=0;
	while(i<6){
		buf[ETH_ARP_DST_MAC_P+i]=buf[ETH_ARP_SRC_MAC_P+i];
		buf[ETH_ARP_SRC_MAC_P+i]=macaddr[i];
		i++;
	}
	i=0;
	while(i<4){
		buf[ETH_ARP_DST_IP_P+i]=buf[ETH_ARP_SRC_IP_P+i];
		buf[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
		i++;
	}
	enc28j60PacketSend(42,buf); 
}

void make_echo_reply_from_request(byte *buf,uint16_t len) {
	make_eth(buf);
	make_ip(buf);
	buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;
	if (buf[ICMP_CHECKSUM_P] > (0xff-0x08))
		buf[ICMP_CHECKSUM_P+1]++;
	buf[ICMP_CHECKSUM_P]+=0x08;
	//
	enc28j60PacketSend(len,buf);
}

void make_udp_reply_from_request(byte *buf,char *data,byte datalen,uint16_t port) {
	make_eth(buf);
	if (datalen>220)
		datalen=220;
	buf[IP_TOTLEN_H_P]=0;
	buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
	make_ip(buf);
	buf[UDP_DST_PORT_H_P]=buf[UDP_SRC_PORT_H_P];
	buf[UDP_DST_PORT_L_P]= buf[UDP_SRC_PORT_L_P];
	buf[UDP_SRC_PORT_H_P]=port>>8;
	buf[UDP_SRC_PORT_L_P]=port;
	buf[UDP_LEN_H_P]=0;
	buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
	buf[UDP_CHECKSUM_H_P]=0;
	buf[UDP_CHECKSUM_L_P]=0;
	byte i=0;
	while(i<datalen){
		buf[UDP_DATA_P+i]=data[i];
		i++;
	}
	uint16_t ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
	buf[UDP_CHECKSUM_H_P]=ck>>8;
	buf[UDP_CHECKSUM_L_P]=ck;
	enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

static void make_tcp_synack_from_syn(byte *buf) {
	make_eth(buf);
	buf[IP_TOTLEN_H_P]=0;
	buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4;
	make_ip(buf);
	buf[TCP_FLAGS_P]=TCP_FLAGS_SYNACK_V;
	make_tcphead(buf,1,0);
	buf[TCP_SEQ_H_P+0]= 0;
	buf[TCP_SEQ_H_P+1]= 0;
	buf[TCP_SEQ_H_P+2]= seqnum; 
	buf[TCP_SEQ_H_P+3]= 0;
	seqnum+=3;
	buf[TCP_OPTIONS_P]=2;
	buf[TCP_OPTIONS_P+1]=4;
	buf[TCP_OPTIONS_P+2]=0x05;
	buf[TCP_OPTIONS_P+3]=0x0;
	buf[TCP_HEADER_LEN_P]=0x60;
	buf[TCP_WIN_SIZE]=0x5; // 1400=0x578
	buf[TCP_WIN_SIZE+1]=0x78;
	uint16_t ck=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+4,2);
	buf[TCP_CHECKSUM_H_P]=ck>>8;
	buf[TCP_CHECKSUM_L_P]=ck;
	enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4+ETH_HEADER_LEN,buf);
}

static uint16_t get_tcp_data_len(byte *buf) {
	int16_t i = (((int16_t)buf[IP_TOTLEN_H_P])<<8)|buf[IP_TOTLEN_L_P];
	i-=IP_HEADER_LEN;
	i-=(buf[TCP_HEADER_LEN_P]>>4)*4; // generate len in bytes;
	if (i<=0)
		i=0;
	return (uint16_t)i;
}

uint16_t fill_tcp_data_p(byte *buf,uint16_t pos, const prog_char *progmem_s) {
	char c;
	while ((c = pgm_read_byte(progmem_s++)))
		buf[TCP_CHECKSUM_L_P+3+pos++]=c;
	return pos;
}

uint16_t fill_tcp_data_len(byte *buf,uint16_t pos, const byte *s, byte len) {
	while (len--)
		buf[TCP_CHECKSUM_L_P+3+pos++]=*s++;
	return pos;
}

uint16_t fill_tcp_data(byte *buf,uint16_t pos, const char *s) {
	return fill_tcp_data_len(buf,pos,(byte*)s,strlen(s));
}

static void make_tcp_ack_from_any(byte *buf,int16_t datlentoack,byte addflags) {
	uint16_t j;
	make_eth(buf);
	buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|addflags;
	if (addflags!=TCP_FLAGS_RST_V && datlentoack==0)
		datlentoack=1;
	make_tcphead(buf,datlentoack,1); // no options
	j=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN;
	buf[IP_TOTLEN_H_P]=j>>8;
	buf[IP_TOTLEN_L_P]=j;
	make_ip(buf);
	buf[TCP_WIN_SIZE]=0x4; // 1024=0x400, 1280=0x500 2048=0x800 768=0x300
	buf[TCP_WIN_SIZE+1]=0;
	j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN,2);
	buf[TCP_CHECKSUM_H_P]=j>>8;
	buf[TCP_CHECKSUM_L_P]=j;
	enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+ETH_HEADER_LEN,buf);
}

static void make_tcp_ack_with_data_noflags(byte *buf,uint16_t dlen) {
	uint16_t j = IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen;
	buf[IP_TOTLEN_H_P]=j>>8;
	buf[IP_TOTLEN_L_P]=j;
	fill_ip_hdr_checksum(buf);
	buf[TCP_CHECKSUM_H_P]=0;
	buf[TCP_CHECKSUM_L_P]=0;
	j=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+dlen,2);
	buf[TCP_CHECKSUM_H_P]=j>>8;
	buf[TCP_CHECKSUM_L_P]=j;
	enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+dlen+ETH_HEADER_LEN,buf);
}

void www_server_reply(byte *buf,uint16_t dlen) {
	make_tcp_ack_from_any(buf,info_data_len,0); // send ack for http get
	buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V|TCP_FLAGS_FIN_V;
	make_tcp_ack_with_data_noflags(buf,dlen); // send data
}

static void fill_buf_p(byte *buf,uint16_t len, const prog_char *progmem_s) {
	while (len--)
		*buf++ = pgm_read_byte(progmem_s++);
}

void client_icmp_request(byte *buf,byte *destip) {
	byte i=0;
	while(i<6){
		buf[ETH_DST_MAC +i]=gwmacaddr[i]; // gw mac in local lan or host mac
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
	buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
	buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
	fill_buf_p(&buf[IP_P],9,iphdr);
	buf[IP_TOTLEN_L_P]=0x54;
	buf[IP_PROTO_P]=IP_PROTO_ICMP_V;
	i=0;
	while(i<4){
		buf[IP_DST_P+i]=destip[i];
		buf[IP_SRC_P+i]=ipaddr[i];
		i++;
	}
	fill_ip_hdr_checksum(buf);
	buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREQUEST_V;
	buf[ICMP_TYPE_P+1]=0; // code
	buf[ICMP_CHECKSUM_H_P]=0;
	buf[ICMP_CHECKSUM_L_P]=0;
	buf[ICMP_IDENT_H_P]=5; // some number 
	buf[ICMP_IDENT_L_P]=ipaddr[3]; // last byte of my IP
	buf[ICMP_IDENT_L_P+1]=0; // seq number, high byte
	buf[ICMP_IDENT_L_P+2]=1; // seq number, low byte, we send only 1 ping at a time
	i=0;
	while(i<56){ 
		buf[ICMP_DATA_P+i]=PINGPATTERN;
		i++;
	}
	uint16_t ck=checksum(&buf[ICMP_TYPE_P], 56+8,0);
	buf[ICMP_CHECKSUM_H_P]=ck>>8;
	buf[ICMP_CHECKSUM_L_P]=ck;
	enc28j60PacketSend(98,buf);
}

void client_ntp_request(byte *buf,byte *ntpip,byte srcport) {
	byte i=0;
	while(i<6){
		buf[ETH_DST_MAC +i]=gwmacaddr[i]; // gw mac in local lan or host mac
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
	buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
	buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
	fill_buf_p(&buf[IP_P],9,iphdr);
	buf[IP_TOTLEN_L_P]=0x4c;
	buf[IP_PROTO_P]=IP_PROTO_UDP_V;
	i=0;
	while(i<4){
		buf[IP_DST_P+i]=ntpip[i];
		buf[IP_SRC_P+i]=ipaddr[i];
		i++;
	}
	fill_ip_hdr_checksum(buf);
	buf[UDP_DST_PORT_H_P]=0;
	buf[UDP_DST_PORT_L_P]=0x7b; // ntp=123
	buf[UDP_SRC_PORT_H_P]=10;
	buf[UDP_SRC_PORT_L_P]=srcport; // lower 8 bit of src port
	buf[UDP_LEN_H_P]=0;
	buf[UDP_LEN_L_P]=56; // fixed len
	buf[UDP_CHECKSUM_H_P]=0;
	buf[UDP_CHECKSUM_L_P]=0;
	i=0;
	while(i<48){ 
		buf[UDP_DATA_P+i]=0;
		i++;
	}
	fill_buf_p(&buf[UDP_DATA_P],10,ntpreqhdr);
	uint16_t ck=checksum(&buf[IP_SRC_P], 16 + 48,1);
	buf[UDP_CHECKSUM_H_P]=ck>>8;
	buf[UDP_CHECKSUM_L_P]=ck;
	enc28j60PacketSend(90,buf);
}

byte client_ntp_process_answer(byte *buf,uint32_t *time,byte dstport_l){
	if (dstport_l && buf[UDP_DST_PORT_L_P]!=dstport_l)
		return 0;
	if (buf[UDP_LEN_H_P]!=0 || buf[UDP_LEN_L_P]!=56 || buf[UDP_SRC_PORT_L_P]!=0x7b)
		return 0;
	*time=((uint32_t)buf[0x52]<<24)|((uint32_t)buf[0x53]<<16)|((uint32_t)buf[0x54]<<8)|((uint32_t)buf[0x55]);
	return 1;
}

void send_udp_prepare(byte *buf,uint16_t sport, byte *dip, uint16_t dport) {
	byte i=0;
	while(i<6){
		buf[ETH_DST_MAC +i]=gwmacaddr[i]; // gw mac in local lan or host mac
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
	buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
	buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
	fill_buf_p(&buf[IP_P],9,iphdr);
	buf[IP_TOTLEN_H_P]=0;
	buf[IP_PROTO_P]=IP_PROTO_UDP_V;
	i=0;
	while(i<4){
		buf[IP_DST_P+i]=dip[i];
		buf[IP_SRC_P+i]=ipaddr[i];
		i++;
	}
	buf[UDP_DST_PORT_H_P]=(dport>>8);
	buf[UDP_DST_PORT_L_P]=0xff&dport; 
	buf[UDP_SRC_PORT_H_P]=(sport>>8);
	buf[UDP_SRC_PORT_L_P]=sport; 
	buf[UDP_LEN_H_P]=0;
	buf[UDP_CHECKSUM_H_P]=0;
	buf[UDP_CHECKSUM_L_P]=0;
}

void send_udp_transmit(byte *buf,byte datalen) {
	buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
	fill_ip_hdr_checksum(buf);
	buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
	uint16_t ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
	buf[UDP_CHECKSUM_H_P]=ck>>8;
	buf[UDP_CHECKSUM_L_P]=ck;
	enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

void send_udp(byte *buf,char *data,byte datalen,uint16_t sport, byte *dip, uint16_t dport) {
	send_udp_prepare(buf,sport, dip, dport);
	byte i=0;
	if (datalen>220)
		datalen=220;
	i=0;
	while(i<datalen){
		buf[UDP_DATA_P+i]=data[i];
		i++;
	}
	send_udp_transmit(buf,datalen);
}

void send_wol(byte *buf,byte *wolmac) {
	byte i=0;
	byte m=0;
	byte pos=0;
	while(i<6){
		buf[ETH_DST_MAC +i]=0xff;
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
	buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
	buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
	fill_buf_p(&buf[IP_P],9,iphdr);
	buf[IP_TOTLEN_L_P]=0x54;
	buf[IP_PROTO_P]=IP_PROTO_ICMP_V;
	i=0;
	while(i<4){
		buf[IP_SRC_P+i]=ipaddr[i];
		buf[IP_DST_P+i]=0xff;
		i++;
	}
	fill_ip_hdr_checksum(buf);
	buf[UDP_DST_PORT_H_P]=0;
	buf[UDP_DST_PORT_L_P]=0x9; // wol=normally 9
	buf[UDP_SRC_PORT_H_P]=10;
	buf[UDP_SRC_PORT_L_P]=0x42; // source port does not matter
	buf[UDP_LEN_H_P]=0;
	buf[UDP_LEN_L_P]=110; // fixed len
	buf[UDP_CHECKSUM_H_P]=0;
	buf[UDP_CHECKSUM_L_P]=0;
	i=0;
	while(i<6){ 
		buf[UDP_DATA_P+i]=0xff;
		i++;
	}
	m=0;
	pos=UDP_DATA_P+i;
	while (m<16){
		i=0;
		while(i<6){ 
			buf[pos]=wolmac[i];
			i++;
			pos++;
		}
		m++;
	}
	uint16_t ck=checksum(&buf[IP_SRC_P], 16+ 102,1);
	buf[UDP_CHECKSUM_H_P]=ck>>8;
	buf[UDP_CHECKSUM_L_P]=ck;
	enc28j60PacketSend(pos,buf);
}

// make a arp request
void client_arp_whohas(byte *buf,byte *ip_we_search) {
	byte i=0;
	while(i<6){
		buf[ETH_DST_MAC +i]=0xff;
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
	buf[ETH_TYPE_H_P] = ETHTYPE_ARP_H_V;
	buf[ETH_TYPE_L_P] = ETHTYPE_ARP_L_V;
	fill_buf_p(&buf[ETH_ARP_P],8,arpreqhdr);
	i=0;
	while(i<6){
		buf[ETH_ARP_SRC_MAC_P +i]=macaddr[i];
		buf[ETH_ARP_DST_MAC_P+i]=0;
		i++;
	}
	i=0;
	while(i<4){
		buf[ETH_ARP_DST_IP_P+i]=*(ip_we_search +i);
		buf[ETH_ARP_SRC_IP_P+i]=ipaddr[i];
		i++;
	}
	waitgwmac|=WGW_ACCEPT_ARP_REPLY;
	enc28j60PacketSend(0x2a,buf);
}

byte client_waiting_gw(void) {
	if (waitgwmac & WGW_HAVE_GW_MAC)
		return 0;
	return 1;
}

static byte client_store_gw_mac(byte *buf) {
	byte i=0;
	while(i<4){
		if (buf[ETH_ARP_SRC_IP_P+i]!=gwip[i])
			return 0;
		i++;
	}
	i=0;
	while(i<6){
		gwmacaddr[i]=buf[ETH_ARP_SRC_MAC_P +i];
		i++;
	}
	return 1;
}

static void client_gw_arp_refresh(void) {
	if (waitgwmac & WGW_HAVE_GW_MAC)
		waitgwmac|=WGW_REFRESHING;
}

void client_set_gwip(byte *gwipaddr) {
	byte i=0;
	waitgwmac=WGW_INITIAL_ARP; // causes an arp request in the packet loop
	while(i<4){
		gwip[i]=gwipaddr[i];
		i++;
	}
}

void client_tcp_set_serverip(byte *ipaddr) {
	byte i=0;
	while(i<4){
		tcpsrvip[i]=ipaddr[i];
		i++;
	}
}

static void client_syn(byte *buf,byte srcport,byte dstport_h,byte dstport_l) {
	byte i=0;
	while(i<6){
		buf[ETH_DST_MAC +i]=gwmacaddr[i]; // gw mac in local lan or host mac
		buf[ETH_SRC_MAC +i]=macaddr[i];
		i++;
	}
	buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
	buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
	fill_buf_p(&buf[IP_P],9,iphdr);
	buf[IP_TOTLEN_L_P]=44; // good for syn
	buf[IP_PROTO_P]=IP_PROTO_TCP_V;
	i=0;
	while(i<4){
		buf[IP_DST_P+i]=tcpsrvip[i];
		buf[IP_SRC_P+i]=ipaddr[i];
		i++;
	}
	fill_ip_hdr_checksum(buf);
	buf[TCP_DST_PORT_H_P]=dstport_h;
	buf[TCP_DST_PORT_L_P]=dstport_l;
	buf[TCP_SRC_PORT_H_P]=TCPCLIENT_SRC_PORT_H;
	buf[TCP_SRC_PORT_L_P]=srcport; // lower 8 bit of src port
	i=0;
	while(i<8){
		buf[TCP_SEQ_H_P+i]=0;
		i++;
	}
	buf[TCP_SEQ_H_P+2]= seqnum; 
	seqnum+=3;
	buf[TCP_HEADER_LEN_P]=0x60; // 0x60=24 len: (0x60>>4) * 4
	buf[TCP_FLAGS_P]=TCP_FLAGS_SYN_V;
	buf[TCP_WIN_SIZE]=0x3; // 1024=0x400 768=0x300, initial window
	buf[TCP_WIN_SIZE+1]=0x0;
	buf[TCP_CHECKSUM_H_P]=0;
	buf[TCP_CHECKSUM_L_P]=0;
	buf[TCP_CHECKSUM_L_P+1]=0;
	buf[TCP_CHECKSUM_L_P+2]=0;
	buf[TCP_OPTIONS_P]=2;
	buf[TCP_OPTIONS_P+1]=4;
	buf[TCP_OPTIONS_P+2]=(CLIENTMSS>>8);
	buf[TCP_OPTIONS_P+3]=CLIENTMSS;
	uint16_t ck=checksum(&buf[IP_SRC_P], 8 +TCP_HEADER_LEN_PLAIN+4,2);
	buf[TCP_CHECKSUM_H_P]=ck>>8;
	buf[TCP_CHECKSUM_L_P]=ck;
	// 4 is the tcp mss option:
	enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+ETH_HEADER_LEN+4,buf);
}

byte client_tcp_req(byte (*result_callback)(byte fd,byte statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(byte fd),uint16_t port) {
	client_tcp_result_callback=result_callback;
	client_tcp_datafill_callback=datafill_callback;
	tcp_client_port_h=port>>8;
	tcp_client_port_l=port;
	tcp_client_state=1; // send a syn
    tcp_fd = (tcp_fd + 1) & 7;
	return tcp_fd;
}

static uint16_t www_client_internal_datafill_callback(byte fd){
	char strbuf[5];
	uint16_t len=0;
	if (fd==www_fd){
		if (browsertype==0){
			len=fill_tcp_data_p(bufptr,0,PSTR("GET "));
			len=fill_tcp_data_p(bufptr,len,client_urlbuf);
			len=fill_tcp_data(bufptr,len,client_urlbuf_var);
			len=fill_tcp_data_p(bufptr,len,PSTR(" HTTP/1.1\r\nHost: "));
			len=fill_tcp_data_p(bufptr,len,client_hoststr);
			len=fill_tcp_data_p(bufptr,len,PSTR("\r\nUser-Agent: tgr/1.0\r\nAccept: text/html\r\nConnection: close\r\n\r\n"));
		}else{
			len=fill_tcp_data_p(bufptr,0,PSTR("POST "));
			len=fill_tcp_data_p(bufptr,len,client_urlbuf);
			len=fill_tcp_data_p(bufptr,len,PSTR(" HTTP/1.1\r\nHost: "));
			len=fill_tcp_data_p(bufptr,len,client_hoststr);
			if (client_additionalheaderline){
				len=fill_tcp_data_p(bufptr,len,PSTR("\r\n"));
				len=fill_tcp_data_p(bufptr,len,client_additionalheaderline);
			}
			len=fill_tcp_data_p(bufptr,len,PSTR("\r\nUser-Agent: tgr/1.1\r\nAccept: */*\r\nConnection: close\r\n"));
			len=fill_tcp_data_p(bufptr,len,PSTR("Content-Length: "));
			itoa(strlen(client_postval),strbuf,10);
			len=fill_tcp_data(bufptr,len,strbuf);
			len=fill_tcp_data_p(bufptr,len,PSTR("\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n"));
			len=fill_tcp_data(bufptr,len,client_postval);
		}
	}
	return len;
}

static byte www_client_internal_result_callback(byte fd, byte statuscode, uint16_t datapos, uint16_t len_of_data){
	if (fd!=www_fd)
		(*client_browser_callback)(4,0,0);
	else if (statuscode==0 && len_of_data>12 && client_browser_callback){
        byte f = strncmp("200",(char *)&(bufptr[datapos+9]),3) != 0;
		(*client_browser_callback)(f, ((uint16_t)TCP_SRC_PORT_H_P+(bufptr[TCP_HEADER_LEN_P]>>4)*4),len_of_data);
	}
	return 0;
}

void client_browse_url(prog_char *urlbuf, char *urlbuf_varpart, prog_char *hoststr,void (*callback)(byte,uint16_t,uint16_t)) {
	client_urlbuf=urlbuf;
	client_urlbuf_var=urlbuf_varpart;
	client_hoststr=hoststr;
	browsertype=0;
	client_browser_callback=callback;
	www_fd=client_tcp_req(&www_client_internal_result_callback,&www_client_internal_datafill_callback,80);
}

void client_http_post(prog_char *urlbuf, prog_char *hoststr, prog_char *additionalheaderline,char *postval,void (*callback)(byte,uint16_t,uint16_t)) {
	client_urlbuf=urlbuf;
	client_hoststr=hoststr;
	client_additionalheaderline=additionalheaderline;
	client_postval=postval;
	browsertype=1;
	client_browser_callback=callback;
	www_fd=client_tcp_req(&www_client_internal_result_callback,&www_client_internal_datafill_callback,80);
}

void register_ping_rec_callback(void (*callback)(byte *srcip)) {
	icmp_callback=callback;
}

byte packetloop_icmp_checkreply(byte *buf,byte *ip_monitoredhost) {
	return buf[IP_PROTO_P]==IP_PROTO_ICMP_V &&
	        buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREPLY_V &&
	         buf[ICMP_DATA_P]== PINGPATTERN &&
              check_ip_message_is_from(buf,ip_monitoredhost);
}

uint16_t packetloop_icmp_tcp(byte *buf,uint16_t plen) {
	uint16_t len;
	byte send_fin=0;
	uint16_t tcpstart;
	uint16_t save_len;
	if (plen==0){
		if ((waitgwmac & WGW_INITIAL_ARP||waitgwmac & WGW_REFRESHING) && delaycnt==0&& enc28j60linkup())
			client_arp_whohas(buf,gwip);
		delaycnt++;
		if (tcp_client_state==1 && (waitgwmac & WGW_HAVE_GW_MAC)){ // send a syn
			tcp_client_state=2;
			tcpclient_src_port_l++; // allocate a new port
			client_syn(buf,((tcp_fd<<5) | (0x1f & tcpclient_src_port_l)),tcp_client_port_h,tcp_client_port_l);
		}
		return 0;
	}
	if (eth_type_is_arp_and_my_ip(buf,plen)){
		if (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REQ_L_V)
			make_arp_answer_from_request(buf);
		if (waitgwmac & WGW_ACCEPT_ARP_REPLY && (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REPLY_L_V) && client_store_gw_mac(buf))
			waitgwmac=WGW_HAVE_GW_MAC;
		return 0;
	}
	if (eth_type_is_ip_and_my_ip(buf,plen)==0)
		return 0;
	if (buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
		if (icmp_callback)
			(*icmp_callback)(&(buf[IP_SRC_P]));
		make_echo_reply_from_request(buf,plen);
		return 0;
	}
	if (plen<54 && buf[IP_PROTO_P]!=IP_PROTO_TCP_V )
		return 0;
	if ( buf[TCP_DST_PORT_H_P]==TCPCLIENT_SRC_PORT_H){
		bufptr=buf; 
		if (check_ip_message_is_from(buf,tcpsrvip)==0)
			return 0;
		if (buf[TCP_FLAGS_P] & TCP_FLAGS_RST_V){
			if (client_tcp_result_callback)
	(*client_tcp_result_callback)((buf[TCP_DST_PORT_L_P]>>5)&0x7,3,0,0);
			tcp_client_state=5;
			return 0;
		}
		len=get_tcp_data_len(buf);
		if (tcp_client_state==2){
			if ((buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V) && (buf[TCP_FLAGS_P] &TCP_FLAGS_ACK_V)){
				make_tcp_ack_from_any(buf,0,0);
				buf[TCP_FLAGS_P]=TCP_FLAGS_ACK_V|TCP_FLAGS_PUSH_V;
				if (client_tcp_datafill_callback)
	len=(*client_tcp_datafill_callback)((buf[TCP_SRC_PORT_L_P]>>5)&0x7);
				else
					len=0;
				tcp_client_state=3;
				make_tcp_ack_with_data_noflags(buf,len);
				return 0;
			}else{
				tcp_client_state=1; // retry
				len++;
				if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V)
					len=0;
				make_tcp_ack_from_any(buf,len,TCP_FLAGS_RST_V);
				return 0;
			}
		}
		if (tcp_client_state==3 && len>0){ 
			tcp_client_state=4;
			if (client_tcp_result_callback){
				tcpstart=TCP_DATA_START; // TCP_DATA_START is a formula
				if (tcpstart>plen-8)
					tcpstart=plen-8; // dummy but save
				save_len=len;
				if (tcpstart+len>plen)
					save_len=plen-tcpstart;
				send_fin = (*client_tcp_result_callback)((buf[TCP_DST_PORT_L_P]>>5)&0x7,0,tcpstart,save_len);
			}
			if (send_fin){
				make_tcp_ack_from_any(buf,len,TCP_FLAGS_PUSH_V|TCP_FLAGS_FIN_V);
				tcp_client_state=5;
				return 0;
			}
		}
		if (tcp_client_state==5)
			return 0;
		if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V){
			make_tcp_ack_from_any(buf,len+1,TCP_FLAGS_PUSH_V|TCP_FLAGS_FIN_V);
			tcp_client_state=5; // connection terminated
			return 0;
		}
		if (len>0)
			make_tcp_ack_from_any(buf,len,0);
		return 0;
	}
	if (buf[TCP_DST_PORT_H_P]==wwwport_h && buf[TCP_DST_PORT_L_P]==wwwport_l){
		if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V){
			make_tcp_synack_from_syn(buf);
			return 0;
		}
		if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V){
			info_data_len=get_tcp_data_len(buf);
			if (info_data_len==0){
				if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V)
					make_tcp_ack_from_any(buf,0,0);
				return 0;
			}
			len=TCP_DATA_START; // TCP_DATA_START is a formula
			if (len>plen-8)
				return 0;
			return len;
		}
	}
	return 0;
}
