#ifndef Stash_h
#define Stash_h

#include "EtherCard.h"

/** This structure describes the structure of memory used within the ENC28J60 network interface. */
typedef struct {
    uint8_t count;     ///< Number of allocated pages
    uint8_t first;     ///< First allocated page
    uint8_t last;      ///< Last allocated page
} StashHeader;

/** This class provides access to the memory within the ENC28J60 network interface. */
class Stash : public /*Stream*/ Print, private StashHeader {
    uint8_t curr;      //!< Current page
    uint8_t offs;      //!< Current offset in page

    typedef struct {
        union {
            uint8_t bytes[64];
            uint16_t words[32];
            struct {
                StashHeader head; // StashHeader is only stored in first block
                uint8_t filler[59];
                uint8_t tail;     // only meaningful if bnum==last; number of bytes in last block
                uint8_t next;     // pointer to next block
            };
        };
        uint8_t bnum;
    } Block;

    static uint8_t allocBlock ();
    static void freeBlock (uint8_t block);
    static uint8_t fetchByte (uint8_t blk, uint8_t off);

    static Block bufs[2];
    static uint8_t map[SCRATCH_MAP_SIZE];

public:
    static void initMap (uint8_t last=SCRATCH_PAGE_NUM);
    static void load (uint8_t idx, uint8_t blk);
    static uint8_t freeCount ();

    Stash () : curr (0) { first = 0; }
    Stash (uint8_t fd) { open(fd); }

    uint8_t create ();
    uint8_t open (uint8_t blk);
    void save ();
    void release ();

    void put (char c);
    char get ();
    uint16_t size ();

    virtual WRITE_RESULT write(uint8_t b) { put(b); WRITE_RETURN }

    // virtual int available() {
    //   if (curr != last)
    //     return 1;
    //   load(1, last);
    //   return offs < bufs[1].tail;
    // }
    // virtual int read() {
    //   return available() ? get() : -1;
    // }
    // virtual int peek() {
    //   return available() ? bufs[1].bytes[offs] : -1;
    // }
    // virtual void flush() {
    //   curr = last;
    //   offs = 63;
    // }

    static void prepare (const char* fmt PROGMEM, ...);
    static uint16_t length ();
    static void extract (uint16_t offset, uint16_t count, void* buf);
    static void cleanup ();

    friend void dumpBlock (const char* msg, uint8_t idx); // optional
    friend void dumpStash (const char* msg, void* ptr);   // optional
};

#endif
