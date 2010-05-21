// Some common utilities needed for IP and web applications
// Author: Guido Socher 
// Copyright: GPL V2
//
// Mods bij jcw, 2010-05-20

#ifndef WEBSRV_HELP_FUNCTIONS_H
#define WEBSRV_HELP_FUNCTIONS_H

// These functions are documented in websrv_help_functions.c
extern uint8_t find_key_val(const char*,char*, uint8_t,const char*);
extern void urldecode(char*);
extern void urlencode(char*,char*);
extern uint8_t parse_ip(uint8_t*,char*);
extern void mk_net_str(char*,uint8_t*,uint8_t,char,uint8_t);

class WebUtil {
public:
    
	uint8_t findKeyVal(const char *str,char *strbuf, uint8_t maxlen, const char *key){
    	return find_key_val(str,strbuf, maxlen,key);
    }

	void urlDecode(char *urlbuf){
    	urldecode(urlbuf);
    }

	void urlEncode(char *str,char *urlbuf){
    	urlencode(str,urlbuf);
    }

	uint8_t parseIp(uint8_t *bytestr,char *str){
    	return parse_ip(bytestr,str);
    }

	void makeNetStr(char *rs,uint8_t *bs,uint8_t len,char sep,uint8_t base){
    	mk_net_str(rs,bs,len,sep,base);
    }
};

#endif /* WEBSRV_HELP_FUNCTIONS_H */
