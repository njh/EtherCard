#include "EtherCard.h"

struct ArpEntry
{
    uint8_t ip[IP_LEN];
    uint8_t mac[ETH_LEN];
    uint8_t count;
};

static ArpEntry store[ETHERCARD_ARP_STORE_SIZE];

static void incArpEntry(ArpEntry &e)
{
    if (e.count < 0xFF)
        ++e.count;
}

// static ArpEntry print_store()
// {
//     Serial.println("ARP store: ");
//     for (ArpEntry *iter = store, *last = store + ETHERCARD_ARP_STORE_SIZE;
//             iter != last; ++iter)
//     {
//         ArpEntry &e = *iter;
//         Serial.print('\t');
//         EtherCard::printIp(e.ip);
//         Serial.print(' ');
//         for (int i = 0; i < ETH_LEN; ++i)
//         {
//             if (i > 0)
//                 Serial.print(':');
//             Serial.print(e.mac[i], HEX);
//         }
//         Serial.print(' ');
//         Serial.println(e.count);
//     }
// }

static ArpEntry *findArpStoreEntry(const uint8_t *ip)
{
    for (ArpEntry *iter = store, *last = store + ETHERCARD_ARP_STORE_SIZE;
            iter != last; ++iter)
    {
        if (memcmp(ip, iter->ip, IP_LEN) == 0)
            return iter;
    }
    return NULL;
}

bool EtherCard::arpStoreHasMac(const uint8_t *ip)
{
    return findArpStoreEntry(ip) != NULL;
}

const uint8_t *EtherCard::arpStoreGetMac(const uint8_t *ip)
{
    ArpEntry *e = findArpStoreEntry(ip);
    if (e)
        return e->mac;
    return NULL;
}

void EtherCard::arpStoreSet(const uint8_t *ip, const uint8_t *mac)
{
    ArpEntry *e = findArpStoreEntry(ip);
    if (!e)
    {
        // find less used entry
        e = store;
        for (ArpEntry *iter = store + 1, *last = store + ETHERCARD_ARP_STORE_SIZE;
                iter != last; ++iter)
        {
            if (iter->count < e->count)
                e = iter;
        }

        // and replace it with new ip/mac
        copyIp(e->ip, ip);
        e->count = 1;
    }
    else
        incArpEntry(*e);

    copyMac(e->mac, mac);
    // print_store();
}

void EtherCard::arpStoreInvalidIp(const uint8_t *ip)
{
    ArpEntry *e = findArpStoreEntry(ip);
    if (e)
        memset(e, 0, sizeof(ArpEntry));
}
