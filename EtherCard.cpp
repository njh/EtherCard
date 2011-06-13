#include <EtherCard.h>
#include <stdarg.h>
#include <avr/eeprom.h>

void BufferFiller::emit_p(PGM_P fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (;;) {
        char c = pgm_read_byte(fmt++);
        if (c == 0)
            break;
        if (c != '$') {
            *ptr++ = c;
            continue;
        }
        c = pgm_read_byte(fmt++);
        switch (c) {
            case 'D':
                itoa(va_arg(ap, int), (char*) ptr, 10);
                break;
            case 'S':
                strcpy((char*) ptr, va_arg(ap, const char*));
                break;
            case 'F': {
                PGM_P s = va_arg(ap, PGM_P);
                char c;
                while ((c = pgm_read_byte(s++)) != 0)
                    *ptr++ = c;
                continue;
            }
            case 'E': {
                byte* s = va_arg(ap, byte*);
                char c;
                while ((c = eeprom_read_byte(s++)) != 0)
                    *ptr++ = c;
                continue;
            }
            default:
                *ptr++ = c;
                continue;
        }
        ptr += strlen((char*) ptr);
    }
    va_end(ap);
}

EtherCard ether;

uint8_t EtherCard::mymac[6];  // my MAC address
uint8_t EtherCard::myip[4];   // my ip address
uint8_t EtherCard::mymask[4]; // my net mask
uint8_t EtherCard::gwip[4];   // gateway
uint8_t EtherCard::dnsip[4];  // dns server
uint8_t EtherCard::hisip[4];  // dns result
uint16_t EtherCard::hisport;  // tcp port to browse to
