// Based on the net.h file from the AVRlib library by Pascal Stang.
// Author: Guido Socher
// Copyright: GPL V2
//
// For AVRlib See http://www.procyonengineering.com/
// Used with explicit permission of Pascal Stang.
//
// 2010-05-20 <jc@wippler.nl>

// notation: _P = position of a field
//           _V = value of a field

#ifndef NET_H
#define NET_H

// macro to swap bytes from 2 bytes value
#define BSWAP_16(x)                         \
    (                                       \
        (uint16_t)                          \
        (                                   \
            ((x & 0xFF) << 8)               \
            | ((x >> 8) & 0xFF)             \
        )                                   \
    )

// macro to swap bytes from 4 bytes value
#define BSWAP_32(x)                         \
    (                                       \
        (uint32_t)                          \
        (                                   \
            ((x & 0xFF) << 24)              \
            | ((x & 0xFF00) << 8)           \
            | ((x & 0xFF0000) >> 8)         \
            | ((x & 0xFF000000) >> 24)      \
        )                                   \
    )

#ifndef __BYTE_ORDER__
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define HTONS(x) BSWAP_16(x)
    #define HTONL(x) BSWAP_32(x)
    #define NTOHS(x) BSWAP_16(x)
    #define NTOHL(x) BSWAP_32(x)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define HTONS(x) x
    #define HTONL(x) x
    #define NTOHS(x) x
    #define NTOHL(x) x
#else
#   error __BYTE_ORDER__ not defined! PLease define it for your platform
#endif

inline uint16_t htons(const uint16_t v)
{ return HTONS(v); }

inline uint32_t htonl(const uint32_t v)
{ return HTONL(v); }

inline uint16_t ntohs(const uint16_t v)
{ return NTOHS(v); }

inline uint32_t ntohl(const uint32_t v)
{ return NTOHL(v); }

// ******* SERVICE PORTS *******
#define HTTP_PORT 80
#define DNS_PORT  53
#define NTP_PORT  123

// ******* ETH *******
#define ETH_HEADER_LEN    14
#define ETH_LEN 6
// values of certain bytes:
#define ETHTYPE_ARP_V               HTONS(0x0806)
#define ETHTYPE_IP_V                HTONS(0x0800)
// byte positions in the ethernet frame:
//
// Ethernet type field (2bytes):
#define ETH_TYPE_H_P 12
#define ETH_TYPE_L_P 13
//
#define ETH_DST_MAC 0
#define ETH_SRC_MAC 6


// Ethernet II header
struct EthHeader
{
    uint8_t     thaddr[ETH_LEN]; // target MAC address
    uint8_t     shaddr[ETH_LEN]; // source MAC address
    uint16_t    etype; // Ethertype
};


// ******* IP *******
#define IP_LEN 4
#define IP_V4 0x4
#define IP_IHL 0x5

enum IpFlags
{
    IP_DF = 1 << 1,
    IP_MF = 1 << 0,
};

#define IP_PROTO_ICMP_V 1
#define IP_PROTO_TCP_V 6
// 17=0x11
#define IP_PROTO_UDP_V 17

struct IpHeader
{
    uint8_t     versionIhl;
    uint8_t     dscpEcn;
    uint16_t    totalLen;
    uint16_t    identification;
    uint16_t    flagsFragmentOffset;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    hchecksum;
    uint8_t     spaddr[IP_LEN];
    uint8_t     tpaddr[IP_LEN];

    static uint8_t version_mask() { return 0xF0; }
    uint8_t version() const { return (versionIhl & version_mask()) >> 4; }
    void version(const uint8_t v)
    {
        versionIhl &= ~version_mask();
        versionIhl |= (v << 4) & version_mask();
    }

    uint8_t ihl() const { return versionIhl & 0xF; }
    void ihl(const uint8_t i)
    {
        versionIhl &= version_mask();
        versionIhl |= i & ~version_mask();
    }

    static uint16_t flags_mask() { return HTONS(0xE000); }

    void flags(const uint16_t f)
    {
        flagsFragmentOffset &= ~flags_mask();
        flagsFragmentOffset |= HTONS(f << 13) & flags_mask();
    }
    void fragmentOffset(const uint16_t o)
    {
        flagsFragmentOffset &= flags_mask();
        flagsFragmentOffset |= HTONS(o) & ~flags_mask();
    }

} __attribute__((__packed__));


// ******* ARP *******
// ArpHeader.htypeH/L
#define ETH_ARP_HTYPE_ETHERNET      HTONS(0x0001)

// ArpHeader.ptypeH/L
#define ETH_ARP_PTYPE_IPV4          HTONS(0x0800)

// ArpHeader.opcodeH/L
#define ETH_ARP_OPCODE_REPLY        HTONS(0x0002)
#define ETH_ARP_OPCODE_REQ          HTONS(0x0001)

struct ArpHeader
{
    uint16_t    htype; // hardware type
    uint16_t    ptype; // protocol type
    uint8_t     hlen; // hardware address length
    uint8_t     plen; // protocol address length
    uint16_t    opcode; // operation

    // only htype "ethernet" and ptype "IPv4" is supported for the moment,
    // so hardcode addresses' lengths
    uint8_t     shaddr[ETH_LEN];
    uint8_t     spaddr[IP_LEN];
    uint8_t     thaddr[ETH_LEN];
    uint8_t     tpaddr[IP_LEN];
};


// ******* ICMP *******
#define ICMP_TYPE_ECHOREPLY_V 0
#define ICMP_TYPE_ECHOREQUEST_V 8
//
#define ICMP_TYPE_P 0x22
#define ICMP_CHECKSUM_P 0x24
#define ICMP_CHECKSUM_H_P 0x24
#define ICMP_CHECKSUM_L_P 0x25
#define ICMP_IDENT_H_P 0x26
#define ICMP_IDENT_L_P 0x27
#define ICMP_DATA_P 0x2a

// ******* UDP *******
#define UDP_HEADER_LEN    8
//
#define UDP_SRC_PORT_H_P 0x22
#define UDP_SRC_PORT_L_P 0x23
#define UDP_DST_PORT_H_P 0x24
#define UDP_DST_PORT_L_P 0x25
//
#define UDP_LEN_H_P 0x26
#define UDP_LEN_L_P 0x27
#define UDP_CHECKSUM_H_P 0x28
#define UDP_CHECKSUM_L_P 0x29
#define UDP_DATA_P 0x2a

// ******* TCP *******
#define TCP_SRC_PORT_H_P 0x22
#define TCP_SRC_PORT_L_P 0x23
#define TCP_DST_PORT_H_P 0x24
#define TCP_DST_PORT_L_P 0x25
// the tcp seq number is 4 bytes 0x26-0x29
#define TCP_SEQ_H_P 0x26
#define TCP_SEQACK_H_P 0x2a
// flags: SYN=2
#define TCP_FLAGS_P 0x2f
#define TCP_FLAGS_SYN_V 2
#define TCP_FLAGS_FIN_V 1
#define TCP_FLAGS_RST_V 4
#define TCP_FLAGS_PUSH_V 8
#define TCP_FLAGS_SYNACK_V 0x12
#define TCP_FLAGS_ACK_V 0x10
#define TCP_FLAGS_PSHACK_V 0x18
//  plain len without the options:
#define TCP_HEADER_LEN_PLAIN 20
#define TCP_HEADER_LEN_P 0x2e
#define TCP_WIN_SIZE 0x30
#define TCP_CHECKSUM_H_P 0x32
#define TCP_CHECKSUM_L_P 0x33
#define TCP_OPTIONS_P 0x36
//
#endif
