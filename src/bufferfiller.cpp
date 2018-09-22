#include "bufferfiller.h"

void BufferFiller::emit_p(const char* fmt PROGMEM, ...) {
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
#ifdef __AVR__
            ether.wtoa(va_arg(ap, uint16_t), (char*) ptr);
#else
            ether.wtoa(va_arg(ap, int), (char*) ptr);
#endif
            break;
#ifdef FLOATEMIT
        case 'T':
            dtostrf    (    va_arg(ap, double), 10, 3, (char*)ptr );
            break;
#endif
        case 'H': {
#ifdef __AVR__
            char p1 =  va_arg(ap, uint16_t);
#else
            char p1 =  va_arg(ap, int);
#endif
            char p2;
            p2 = (p1 >> 4) & 0x0F;
            p1 = p1 & 0x0F;
            if (p1 > 9) p1 += 0x07; // adjust 0x0a-0x0f to come out 'a'-'f'
            p1 += 0x30;             // and complete
            if (p2 > 9) p2 += 0x07; // adjust 0x0a-0x0f to come out 'a'-'f'
            p2 += 0x30;             // and complete
            *ptr++ = p2;
            *ptr++ = p1;
            continue;
        }
        case 'L':
            ltoa(va_arg(ap, long), (char*) ptr, 10);
            break;
        case 'S':
            strcpy((char*) ptr, va_arg(ap, const char*));
            break;
        case 'F': {
            const char* s PROGMEM = va_arg(ap, const char*);
            char d;
            while ((d = pgm_read_byte(s++)) != 0)
                *ptr++ = d;
            continue;
        }
        case 'E': {
            byte* s = va_arg(ap, byte*);
            char d;
            while ((d = eeprom_read_byte(s++)) != 0)
                *ptr++ = d;
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

