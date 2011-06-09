// DHCP look-up functions based on the udp client
// http://www.ietf.org/rfc/rfc2131.txt
//
// Author: Andrew Lindsay
// Rewritten and optimized by Jean-Claude Wippler, http://jeelabs.org/
//
// Copyright: GPL V2
// See http://www.gnu.org/licenses/gpl.html

#ifndef DHCP_H
#define DHCP_H 1

typedef struct {
    uint8_t myip[4];    // my ip address
    uint8_t mymask[4];  // my net mask
    uint8_t gwip[4];    // gateway
    uint8_t dnsip[4];   // dns server
} DHCPinfo;
    
class DHCP {
public:
    static uint8_t dhcpInit (uint8_t* macaddr, DHCPinfo& dip);
    static uint8_t dhcpCheck (uint8_t* buf, uint16_t len);
    static void printIP (const char* msg, uint8_t *buf);
};
    
#endif /* DHCP_H */
