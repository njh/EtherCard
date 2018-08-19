#include <stdarg.h>
#include <avr/eeprom.h>

#include "stash.h"

#define WRITEBUF  0
#define READBUF   1
#define BUFCOUNT  2

//#define FLOATEMIT // uncomment line to enable $T in emit_P for float emitting

byte Stash::map[SCRATCH_MAP_SIZE];
Stash::Block Stash::bufs[BUFCOUNT];

uint8_t Stash::allocBlock () {
    for (uint8_t i = 0; i < sizeof map; ++i)
        if (map[i] != 0)
            for (uint8_t j = 0; j < 8; ++j)
                if (bitRead(map[i], j)) {
                    bitClear(map[i], j);
                    return (i << 3) + j;
                }
    return 0;
}

void Stash::freeBlock (uint8_t block) {
    bitSet(map[block>>3], block & 7);
}

uint8_t Stash::fetchByte (uint8_t blk, uint8_t off) {
    return blk == bufs[WRITEBUF].bnum ? bufs[WRITEBUF].bytes[off] :
           blk == bufs[READBUF].bnum ? bufs[READBUF].bytes[off] :
           ether.peekin(blk, off);
}


// block 0 is special since always occupied
void Stash::initMap (uint8_t last /*=SCRATCH_PAGE_NUM*/) {
    last = SCRATCH_PAGE_NUM;
    while (--last > 0)
        freeBlock(last);
}

// load a page/block either into the write or into the readbuffer
void Stash::load (uint8_t idx, uint8_t blk) {
    if (blk != bufs[idx].bnum) {
        if (idx == WRITEBUF) {
            ether.copyout(bufs[idx].bnum, bufs[idx].bytes);
            if (blk == bufs[READBUF].bnum)
                bufs[READBUF].bnum = 255; // forget read page if same
        } else if (blk == bufs[WRITEBUF].bnum) {
            // special case: read page is same as write buffer
            memcpy(&bufs[READBUF], &bufs[WRITEBUF], sizeof bufs[0]);
            return;
        }
        bufs[idx].bnum = blk;
        ether.copyin(bufs[idx].bnum, bufs[idx].bytes);
    }
}

uint8_t Stash::freeCount () {
    uint8_t count = 0;
    for (uint8_t i = 0; i < sizeof map; ++i)
        for (uint8_t m = 0x80; m != 0; m >>= 1)
            if (map[i] & m)
                ++count;
    return count;
}

// create a new stash; make it the active stash; return the first block as a handle
uint8_t Stash::create () {
    uint8_t blk = allocBlock();
    load(WRITEBUF, blk);
    bufs[WRITEBUF].head.count = 0;
    bufs[WRITEBUF].head.first = bufs[0].head.last = blk;
    bufs[WRITEBUF].tail = sizeof (StashHeader);
    bufs[WRITEBUF].next = 0;
    return open(blk); // you are now the active stash
}

// the stashheader part only contains reasonable data if we are the first block
uint8_t Stash::open (uint8_t blk) {
    curr = blk;
    offs = sizeof (StashHeader); // goto first byte
    load(READBUF, curr);
    memcpy((StashHeader*) this, bufs[READBUF].bytes, sizeof (StashHeader));
    return curr;
}

// save the metadata of current block into the first block
void Stash::save () {
    load(WRITEBUF, first);
    memcpy(bufs[WRITEBUF].bytes, (StashHeader*) this, sizeof (StashHeader));
    if (bufs[READBUF].bnum == first)
        load(READBUF, 0); // invalidates original in case it was the same block
}

// follow the linked list of blocks and free every block
void Stash::release () {
    while (first > 0) {
        freeBlock(first);
        first = ether.peekin(first, 63);
    }
}

void Stash::put (char c) {
    load(WRITEBUF, last);
    uint8_t t = bufs[WRITEBUF].tail;
    bufs[WRITEBUF].bytes[t++] = c;
    if (t <= 62)
        bufs[WRITEBUF].tail = t;
    else {
        bufs[WRITEBUF].next = allocBlock();
        last = bufs[WRITEBUF].next;
        load(WRITEBUF, last);
        bufs[WRITEBUF].tail = bufs[WRITEBUF].next = 0;
        ++count;
    }
}

char Stash::get () {
    load(READBUF, curr);
    if (curr == last && offs >= bufs[READBUF].tail)
        return 0;
    uint8_t b = bufs[READBUF].bytes[offs];
    if (++offs >= 63 && curr != last) {
        curr = bufs[READBUF].next;
        offs = 0;
    }
    return b;
}

// fetchbyte(last, 62) is tail, i.e., number of characters in last block
uint16_t Stash::size () {
    return 63 * count + fetchByte(last, 62) - sizeof (StashHeader);
}

// write information about the fmt string and the arguments into special page/block 0
// block 0 is initially marked as allocated and never returned by allocateBlock
void Stash::prepare (const char* fmt PROGMEM, ...) {
    Stash::load(WRITEBUF, 0);
    uint16_t* segs = Stash::bufs[WRITEBUF].words;
    *segs++ = strlen_P(fmt);
#ifdef __AVR__
    *segs++ = (uint16_t) fmt;
#else
    *segs++ = (uint32_t) fmt;
    *segs++ = (uint32_t) fmt >> 16;
#endif
    va_list ap;
    va_start(ap, fmt);
    for (;;) {
        char c = pgm_read_byte(fmt++);
        if (c == 0)
            break;
        if (c == '$') {
#ifdef __AVR__
            uint16_t argval = va_arg(ap, uint16_t), arglen = 0;
#else
            uint32_t argval = va_arg(ap, int), arglen = 0;
#endif
            switch (pgm_read_byte(fmt++)) {
            case 'D': {
                char buf[7];
                ether.wtoa(argval, buf);
                arglen = strlen(buf);
                break;
            }
            case 'S':
                arglen = strlen((const char*) argval);
                break;
            case 'F':
                arglen = strlen_P((const char*) argval);
                break;
            case 'E': {
                byte* s = (byte*) argval;
                char d;
                while ((d = eeprom_read_byte(s++)) != 0)
                    ++arglen;
                break;
            }
            case 'H': {
                Stash stash (argval);
                arglen = stash.size();
                break;
            }
            }
#ifdef __AVR__
            *segs++ = argval;
#else
            *segs++ = argval;
            *segs++ = argval >> 16;
#endif
            Stash::bufs[WRITEBUF].words[0] += arglen - 2;
        }
    }
    va_end(ap);
}

uint16_t Stash::length () {
    Stash::load(WRITEBUF, 0);
    return Stash::bufs[WRITEBUF].words[0];
}

void Stash::extract (uint16_t offset, uint16_t count, void* buf) {
    Stash::load(WRITEBUF, 0);
    uint16_t* segs = Stash::bufs[WRITEBUF].words;
#ifdef __AVR__
    const char* fmt PROGMEM = (const char*) *++segs;
#else
    const char* fmt PROGMEM = (const char*)((segs[2] << 16) | segs[1]);
    segs += 2;
#endif
    Stash stash;
    char mode = '@', tmp[7], *ptr = NULL, *out = (char*) buf;
    for (uint16_t i = 0; i < offset + count; ) {
        char c = 0;
        switch (mode) {
        case '@': {
            c = pgm_read_byte(fmt++);
            if (c == 0)
                return;
            if (c != '$')
                break;
#ifdef __AVR__
            uint16_t arg = *++segs;
#else
            uint32_t arg = *++segs;
            arg |= *++segs << 16;
#endif
            mode = pgm_read_byte(fmt++);
            switch (mode) {
            case 'D':
                ether.wtoa(arg, tmp);
                ptr = tmp;
                break;
            case 'S':
            case 'F':
            case 'E':
                ptr = (char*) arg;
                break;
            case 'H':
                stash.open(arg);
                ptr = (char*) &stash;
                break;
            }
            continue;
        }
        case 'D':
        case 'S':
            c = *ptr++;
            break;
        case 'F':
            c = pgm_read_byte(ptr++);
            break;
        case 'E':
            c = eeprom_read_byte((byte*) ptr++);
            break;
        case 'H':
            c = ((Stash*) ptr)->get();
            break;
        }
        if (c == 0) {
            mode = '@';
            continue;
        }
        if (i >= offset)
            *out++ = c;
        ++i;
    }
}

void Stash::cleanup () {
    Stash::load(WRITEBUF, 0);
    uint16_t* segs = Stash::bufs[WRITEBUF].words;
#ifdef __AVR__
    const char* fmt PROGMEM = (const char*) *++segs;
#else
    const char* fmt PROGMEM = (const char*)((segs[2] << 16) | segs[1]);
    segs += 2;
#endif
    for (;;) {
        char c = pgm_read_byte(fmt++);
        if (c == 0)
            break;
        if (c == '$') {
#ifdef __AVR__
            uint16_t arg = *++segs;
#else
            uint32_t arg = *++segs;
            arg |= *++segs << 16;
#endif
            if (pgm_read_byte(fmt++) == 'H') {
                Stash stash (arg);
                stash.release();
            }
        }
    }
}

