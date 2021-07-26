// Microchip ENC28J60 Ethernet Interface Driver
// Author: Guido Socher
// Copyright: GPL V2
//
// Based on the enc28j60.c file from the AVRlib library by Pascal Stang.
// For AVRlib See http://www.procyonengineering.com/
// Used with explicit permission of Pascal Stang.
//
// 2010-05-20 <jc@wippler.nl>

#if ARDUINO >= 100
#include <Arduino.h> // Arduino 1.0
#else
#include <Wprogram.h> // Arduino 0022
#endif
#include "enc28j60.h"

uint16_t ENC28J60::bufferSize;
bool ENC28J60::broadcast_enabled = false;
bool ENC28J60::promiscuous_enabled = false;

// ENC28J60 Control Registers
// Control register definitions are a combination of address,
// bank number, and Ethernet/MAC/PHY indicator bits.
// - Register address        (bits 0-4)
// - Bank number        (bits 5-6)
// - MAC/PHY indicator        (bit 7)
#define ADDR_MASK        0x1F
#define BANK_MASK        0x60
#define SPRD_MASK        0x80
// All-bank registers
#define EIE              0x1B
#define EIR              0x1C
#define ESTAT            0x1D
#define ECON2            0x1E
#define ECON1            0x1F
// Bank 0 registers
#define ERDPT           (0x00|0x00)
#define EWRPT           (0x02|0x00)
#define ETXST           (0x04|0x00)
#define ETXND           (0x06|0x00)
#define ERXST           (0x08|0x00)
#define ERXND           (0x0A|0x00)
#define ERXRDPT         (0x0C|0x00)
// #define ERXWRPT         (0x0E|0x00)
#define EDMAST          (0x10|0x00)
#define EDMAND          (0x12|0x00)
// #define EDMADST         (0x14|0x00)
#define EDMACS          (0x16|0x00)
// Bank 1 registers
#define EHT0             (0x00|0x20)
#define EHT1             (0x01|0x20)
#define EHT2             (0x02|0x20)
#define EHT3             (0x03|0x20)
#define EHT4             (0x04|0x20)
#define EHT5             (0x05|0x20)
#define EHT6             (0x06|0x20)
#define EHT7             (0x07|0x20)
#define EPMM0            (0x08|0x20)
#define EPMM1            (0x09|0x20)
#define EPMM2            (0x0A|0x20)
#define EPMM3            (0x0B|0x20)
#define EPMM4            (0x0C|0x20)
#define EPMM5            (0x0D|0x20)
#define EPMM6            (0x0E|0x20)
#define EPMM7            (0x0F|0x20)
#define EPMCS           (0x10|0x20)
// #define EPMO            (0x14|0x20)
#define EWOLIE           (0x16|0x20)
#define EWOLIR           (0x17|0x20)
#define ERXFCON          (0x18|0x20)
#define EPKTCNT          (0x19|0x20)
// Bank 2 registers
#define MACON1           (0x00|0x40|0x80)
#define MACON3           (0x02|0x40|0x80)
#define MACON4           (0x03|0x40|0x80)
#define MABBIPG          (0x04|0x40|0x80)
#define MAIPG           (0x06|0x40|0x80)
#define MACLCON1         (0x08|0x40|0x80)
#define MACLCON2         (0x09|0x40|0x80)
#define MAMXFL          (0x0A|0x40|0x80)
#define MAPHSUP          (0x0D|0x40|0x80)
#define MICON            (0x11|0x40|0x80)
#define MICMD            (0x12|0x40|0x80)
#define MIREGADR         (0x14|0x40|0x80)
#define MIWR            (0x16|0x40|0x80)
#define MIRD            (0x18|0x40|0x80)
// Bank 3 registers
#define MAADR1           (0x00|0x60|0x80)
#define MAADR0           (0x01|0x60|0x80)
#define MAADR3           (0x02|0x60|0x80)
#define MAADR2           (0x03|0x60|0x80)
#define MAADR5           (0x04|0x60|0x80)
#define MAADR4           (0x05|0x60|0x80)
#define EBSTSD           (0x06|0x60)
#define EBSTCON          (0x07|0x60)
#define EBSTCS          (0x08|0x60)
#define MISTAT           (0x0A|0x60|0x80)
#define EREVID           (0x12|0x60)
#define ECOCON           (0x15|0x60)
#define EFLOCON          (0x17|0x60)
#define EPAUS           (0x18|0x60)

// ENC28J60 ERXFCON Register Bit Definitions
#define ERXFCON_UCEN     0x80
#define ERXFCON_ANDOR    0x40
#define ERXFCON_CRCEN    0x20
#define ERXFCON_PMEN     0x10
#define ERXFCON_MPEN     0x08
#define ERXFCON_HTEN     0x04
#define ERXFCON_MCEN     0x02
#define ERXFCON_BCEN     0x01
// ENC28J60 EIE Register Bit Definitions
#define EIE_INTIE        0x80
#define EIE_PKTIE        0x40
#define EIE_DMAIE        0x20
#define EIE_LINKIE       0x10
#define EIE_TXIE         0x08
#define EIE_WOLIE        0x04
#define EIE_TXERIE       0x02
#define EIE_RXERIE       0x01
// ENC28J60 EIR Register Bit Definitions
#define EIR_PKTIF        0x40
#define EIR_DMAIF        0x20
#define EIR_LINKIF       0x10
#define EIR_TXIF         0x08
#define EIR_WOLIF        0x04
#define EIR_TXERIF       0x02
#define EIR_RXERIF       0x01
// ENC28J60 ESTAT Register Bit Definitions
#define ESTAT_INT        0x80
#define ESTAT_LATECOL    0x10
#define ESTAT_RXBUSY     0x04
#define ESTAT_TXABRT     0x02
#define ESTAT_CLKRDY     0x01
// ENC28J60 ECON2 Register Bit Definitions
#define ECON2_AUTOINC    0x80
#define ECON2_PKTDEC     0x40
#define ECON2_PWRSV      0x20
#define ECON2_VRPS       0x08
// ENC28J60 ECON1 Register Bit Definitions
#define ECON1_TXRST      0x80
#define ECON1_RXRST      0x40
#define ECON1_DMAST      0x20
#define ECON1_CSUMEN     0x10
#define ECON1_TXRTS      0x08
#define ECON1_RXEN       0x04
#define ECON1_BSEL1      0x02
#define ECON1_BSEL0      0x01
// ENC28J60 MACON1 Register Bit Definitions
#define MACON1_LOOPBK    0x10
#define MACON1_TXPAUS    0x08
#define MACON1_RXPAUS    0x04
#define MACON1_PASSALL   0x02
#define MACON1_MARXEN    0x01
// ENC28J60 MACON3 Register Bit Definitions
#define MACON3_PADCFG2   0x80
#define MACON3_PADCFG1   0x40
#define MACON3_PADCFG0   0x20
#define MACON3_TXCRCEN   0x10
#define MACON3_PHDRLEN   0x08
#define MACON3_HFRMLEN   0x04
#define MACON3_FRMLNEN   0x02
#define MACON3_FULDPX    0x01
// ENC28J60 MICMD Register Bit Definitions
#define MICMD_MIISCAN    0x02
#define MICMD_MIIRD      0x01
// ENC28J60 MISTAT Register Bit Definitions
#define MISTAT_NVALID    0x04
#define MISTAT_SCAN      0x02
#define MISTAT_BUSY      0x01

// ENC28J60 EBSTCON Register Bit Definitions
#define EBSTCON_PSV2     0x80
#define EBSTCON_PSV1     0x40
#define EBSTCON_PSV0     0x20
#define EBSTCON_PSEL     0x10
#define EBSTCON_TMSEL1   0x08
#define EBSTCON_TMSEL0   0x04
#define EBSTCON_TME      0x02
#define EBSTCON_BISTST    0x01

// PHY registers
#define PHCON1           0x00
#define PHSTAT1          0x01
#define PHHID1           0x02
#define PHHID2           0x03
#define PHCON2           0x10
#define PHSTAT2          0x11
#define PHIE             0x12
#define PHIR             0x13
#define PHLCON           0x14

// ENC28J60 PHY PHCON1 Register Bit Definitions
#define PHCON1_PRST      0x8000
#define PHCON1_PLOOPBK   0x4000
#define PHCON1_PPWRSV    0x0800
#define PHCON1_PDPXMD    0x0100
// ENC28J60 PHY PHSTAT1 Register Bit Definitions
#define PHSTAT1_PFDPX    0x1000
#define PHSTAT1_PHDPX    0x0800
#define PHSTAT1_LLSTAT   0x0004
#define PHSTAT1_JBSTAT   0x0002
// ENC28J60 PHY PHCON2 Register Bit Definitions
#define PHCON2_FRCLINK   0x4000
#define PHCON2_TXDIS     0x2000
#define PHCON2_JABBER    0x0400
#define PHCON2_HDLDIS    0x0100

// ENC28J60 Packet Control Byte Bit Definitions
#define PKTCTRL_PHUGEEN  0x08
#define PKTCTRL_PPADEN   0x04
#define PKTCTRL_PCRCEN   0x02
#define PKTCTRL_POVERRIDE 0x01

// SPI operation codes
#define ENC28J60_READ_CTRL_REG       0x00
#define ENC28J60_READ_BUF_MEM        0x3A
#define ENC28J60_WRITE_CTRL_REG      0x40
#define ENC28J60_WRITE_BUF_MEM       0x7A
#define ENC28J60_BIT_FIELD_SET       0x80
#define ENC28J60_BIT_FIELD_CLR       0xA0
#define ENC28J60_SOFT_RESET          0xFF

// max frame length which the controller will accept:
// (note: maximum ethernet frame length would be 1518)
#define MAX_FRAMELEN      1500

#define FULL_SPEED  1   // switch to full-speed SPI for bulk transfers

static byte Enc28j60Bank;
static byte selectPin;

void ENC28J60::initSPI () {
    pinMode(selectPin, OUTPUT);
    digitalWrite(selectPin, HIGH);
    pinMode(MOSI, OUTPUT);
    pinMode(SCK, OUTPUT);
    pinMode(MISO, INPUT);

    digitalWrite(MOSI, HIGH);
    digitalWrite(MOSI, LOW);
    digitalWrite(SCK, LOW);

    SPCR = bit(SPE) | bit(MSTR); // 8 MHz @ 16
    bitSet(SPSR, SPI2X);
}

static void enableChip () {
    cli();
    digitalWrite(selectPin, LOW);
}

static void disableChip () {
    digitalWrite(selectPin, HIGH);
    sei();
}

static void xferSPI (byte data) {
    SPDR = data;
    while (!(SPSR&(1<<SPIF)))
        ;
}

static byte readOp (byte op, byte address) {
    enableChip();
    xferSPI(op | (address & ADDR_MASK));
    xferSPI(0x00);
    if (address & 0x80)
        xferSPI(0x00);
    byte result = SPDR;
    disableChip();
    return result;
}

static void writeOp (byte op, byte address, byte data) {
    enableChip();
    xferSPI(op | (address & ADDR_MASK));
    xferSPI(data);
    disableChip();
}

static void readBuf(uint16_t len, byte* data) {
    uint8_t nextbyte;

    enableChip();
    if (len != 0) {
        xferSPI(ENC28J60_READ_BUF_MEM);

        SPDR = 0x00;
        while (--len) {
            while (!(SPSR & (1<<SPIF)))
                ;
            nextbyte = SPDR;
            SPDR = 0x00;
            *data++ = nextbyte;
        }
        while (!(SPSR & (1<<SPIF)))
            ;
        *data++ = SPDR;
    }
    disableChip();
}

static void writeBuf(uint16_t len, const byte* data) {
    enableChip();
    if (len != 0) {
        xferSPI(ENC28J60_WRITE_BUF_MEM);

        SPDR = *data++;
        while (--len) {
            uint8_t nextbyte = *data++;
        	while (!(SPSR & (1<<SPIF)))
                ;
            SPDR = nextbyte;
     	};
        while (!(SPSR & (1<<SPIF)))
            ;
    }
    disableChip();
}

static void SetBank (byte address) {
    if ((address & BANK_MASK) != Enc28j60Bank) {
        writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_BSEL1|ECON1_BSEL0);
        Enc28j60Bank = address & BANK_MASK;
        writeOp(ENC28J60_BIT_FIELD_SET, ECON1, Enc28j60Bank>>5);
    }
}

static byte readRegByte (byte address) {
    SetBank(address);
    return readOp(ENC28J60_READ_CTRL_REG, address);
}

static uint16_t readReg(byte address) {
    return readRegByte(address) + (readRegByte(address+1) << 8);
}

static void writeRegByte (byte address, byte data) {
    SetBank(address);
    writeOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

static void writeReg(byte address, uint16_t data) {
    writeRegByte(address, data);
    writeRegByte(address + 1, data >> 8);
}

static uint16_t readPhyByte (byte address) {
    writeRegByte(MIREGADR, address);
    writeRegByte(MICMD, MICMD_MIIRD);
    while (readRegByte(MISTAT) & MISTAT_BUSY)
        ;
    writeRegByte(MICMD, 0x00);
    return readRegByte(MIRD+1);
}

static void writePhy (byte address, uint16_t data) {
    writeRegByte(MIREGADR, address);
    writeReg(MIWR, data);
    while (readRegByte(MISTAT) & MISTAT_BUSY)
        ;
}

byte ENC28J60::initialize (uint16_t size, const byte* macaddr, byte csPin) {
    bufferSize = size;

    selectPin = csPin;

    if (bitRead(SPCR, SPE) == 0)
        initSPI();

    pinMode(selectPin, OUTPUT);
    disableChip();

    writeOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    delay(2); // errata B7/2
    while (!(readOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_CLKRDY))
        ;

    writeReg(ERXST, RXSTART_INIT);
    writeReg(ERXRDPT, RXSTART_INIT);
    writeReg(ERXND, RXSTOP_INIT);
    writeReg(ETXST, TXSTART_INIT);
    writeReg(ETXND, TXSTOP_INIT);

    // Stretch pulses for LED, LED_A=Link, LED_B=activity
    writePhy(PHLCON, 0x476);

    writeRegByte(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);
    writeReg(EPMM0, 0x303f);
    writeReg(EPMCS, 0xf7f9);
    writeRegByte(MACON1, MACON1_MARXEN);
    writeOp(ENC28J60_BIT_FIELD_SET, MACON3,
            MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
    writeReg(MAIPG, 0x0C12);
    writeRegByte(MABBIPG, 0x12);
    writeReg(MAMXFL, MAX_FRAMELEN);
    writeRegByte(MAADR5, macaddr[0]);
    writeRegByte(MAADR4, macaddr[1]);
    writeRegByte(MAADR3, macaddr[2]);
    writeRegByte(MAADR2, macaddr[3]);
    writeRegByte(MAADR1, macaddr[4]);
    writeRegByte(MAADR0, macaddr[5]);
    writePhy(PHCON2, PHCON2_HDLDIS);
    SetBank(ECON1);
    writeOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
    writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

    byte rev = readRegByte(EREVID);
    // microchip forgot to step the number on the silicon when they
    // released the revision B7. 6 is now rev B7. We still have
    // to see what they do when they release B8. At the moment
    // there is no B8 out yet
    if (rev > 5) ++rev;
    return rev;
}

bool ENC28J60::isLinkUp() {
    return (readPhyByte(PHSTAT2) >> 2) & 1;
}

/*
struct __attribute__((__packed__)) transmit_status_vector {
    uint16_t transmitByteCount;
    byte     transmitCollisionCount      :  4;
    byte     transmitCrcError            :  1;
    byte     transmitLengthCheckError    :  1;
    byte     transmitLengthOutRangeError :  1;
    byte     transmitDone                :  1;
    byte     transmitMulticast           :  1;
    byte     transmitBroadcast           :  1;
    byte     transmitPacketDefer         :  1;
    byte     transmitExcessiveDefer      :  1;
    byte     transmitExcessiveCollision  :  1;
    byte     transmitLateCollision       :  1;
    byte     transmitGiant               :  1;
    byte     transmitUnderrun            :  1;
    uint16_t totalTransmitted;
    byte     transmitControlFrame        :  1;
    byte     transmitPauseControlFrame   :  1;
    byte     backpressureApplied         :  1;
    byte     transmitVLAN                :  1;
    byte     zero                        :  4;
};
*/

struct transmit_status_vector {
    uint8_t bytes[7];
};

#if ETHERCARD_SEND_PIPELINING
    #define BREAKORCONTINUE retry=0; continue;
#else
    #define BREAKORCONTINUE break;
#endif

void ENC28J60::packetSend(uint16_t len) {
    byte retry = 0;

    #if ETHERCARD_SEND_PIPELINING
        goto resume_last_transmission;
    #endif
    while (1) {
        // latest errata sheet: DS80349C
        // always reset transmit logic (Errata Issue 12)
        // the Microchip TCP/IP stack implementation used to first check
        // whether TXERIF is set and only then reset the transmit logic
        // but this has been changed in later versions; possibly they
        // have a reason for this; they don't mention this in the errata
        // sheet
        writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
        writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
        writeOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF|EIR_TXIF);

        // prepare new transmission
        if (retry == 0) {
            writeReg(EWRPT, TXSTART_INIT);
            writeReg(ETXND, TXSTART_INIT+len);
            writeOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
            writeBuf(len, buffer);
        }

        // initiate transmission
        writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
        #if ETHERCARD_SEND_PIPELINING
            if (retry == 0) return;
        #endif

    resume_last_transmission:

        // wait until transmission has finished; referring to the data sheet and
        // to the errata (Errata Issue 13; Example 1) you only need to wait until either
        // TXIF or TXERIF gets set; however this leads to hangs; apparently Microchip
        // realized this and in later implementations of their tcp/ip stack they introduced
        // a counter to avoid hangs; of course they didn't update the errata sheet
        uint16_t count = 0;
        while ((readRegByte(EIR) & (EIR_TXIF | EIR_TXERIF)) == 0 && ++count < 1000U)
            ;

        if (!(readRegByte(EIR) & EIR_TXERIF) && count < 1000U) {
            // no error; start new transmission
            BREAKORCONTINUE
        }

        // cancel previous transmission if stuck
        writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);

    #if ETHERCARD_RETRY_LATECOLLISIONS == 0
        BREAKORCONTINUE
    #endif

        // Check whether the chip thinks that a late collision occurred; the chip
        // may be wrong (Errata Issue 13); therefore we retry. We could check
        // LATECOL in the ESTAT register in order to find out whether the chip
        // thinks a late collision occurred but (Errata Issue 15) tells us that
        // this is not working. Therefore we check TSV
        transmit_status_vector tsv;
        uint16_t etxnd = readReg(ETXND);
        writeReg(ERDPT, etxnd+1);
        readBuf(sizeof(transmit_status_vector), (byte*) &tsv);
        // LATECOL is bit number 29 in TSV (starting from 0)

        if (!((readRegByte(EIR) & EIR_TXERIF) && (tsv.bytes[3] & 1<<5) /*tsv.transmitLateCollision*/) || retry > 16U) {
            // there was some error but no LATECOL so we do not repeat
            BREAKORCONTINUE
        }

        retry++;
    }
}


uint16_t ENC28J60::packetReceive() {
    static uint16_t gNextPacketPtr = RXSTART_INIT;
    static bool     unreleasedPacket = false;
    uint16_t len = 0;

    if (unreleasedPacket) {
        if (gNextPacketPtr == 0)
            writeReg(ERXRDPT, RXSTOP_INIT);
        else
            writeReg(ERXRDPT, gNextPacketPtr - 1);
        unreleasedPacket = false;
    }

    if (readRegByte(EPKTCNT) > 0) {
        writeReg(ERDPT, gNextPacketPtr);

        struct {
            uint16_t nextPacket;
            uint16_t byteCount;
            uint16_t status;
        } header;

        readBuf(sizeof header, (byte*) &header);

        gNextPacketPtr  = header.nextPacket;
        len = header.byteCount - 4; //remove the CRC count
        if (len>bufferSize-1)
            len=bufferSize-1;
        if ((header.status & 0x80)==0)
            len = 0;
        else
            readBuf(len, buffer);
        buffer[len] = 0;
        unreleasedPacket = true;

        writeOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
    }
    return len;
}

void ENC28J60::copyout (byte page, const byte* data) {
    uint16_t destPos = SCRATCH_START + (page << SCRATCH_PAGE_SHIFT);
    if (destPos < SCRATCH_START || destPos > SCRATCH_LIMIT - SCRATCH_PAGE_SIZE)
        return;
    writeReg(EWRPT, destPos);
    writeBuf(SCRATCH_PAGE_SIZE, data);
}

void ENC28J60::copyin (byte page, byte* data) {
    uint16_t destPos = SCRATCH_START + (page << SCRATCH_PAGE_SHIFT);
    if (destPos < SCRATCH_START || destPos > SCRATCH_LIMIT - SCRATCH_PAGE_SIZE)
        return;
    writeReg(ERDPT, destPos);
    readBuf(SCRATCH_PAGE_SIZE, data);
}

byte ENC28J60::peekin (byte page, byte off) {
    byte result = 0;
    uint16_t destPos = SCRATCH_START + (page << SCRATCH_PAGE_SHIFT) + off;
    if (SCRATCH_START <= destPos && destPos < SCRATCH_LIMIT) {
        writeReg(ERDPT, destPos);
        readBuf(1, &result);
    }
    return result;
}

// Contributed by Alex M. Based on code from: http://blog.derouineau.fr
//                  /2011/07/putting-enc28j60-ethernet-controler-in-sleep-mode/
void ENC28J60::powerDown() {
    writeOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);
    while(readRegByte(ESTAT) & ESTAT_RXBUSY);
    while(readRegByte(ECON1) & ECON1_TXRTS);
    writeOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_VRPS);
    writeOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PWRSV);
}

void ENC28J60::powerUp() {
    writeOp(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);
    while(!(readRegByte(ESTAT) & ESTAT_CLKRDY));
    writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

void ENC28J60::enableBroadcast (bool temporary) {
    writeRegByte(ERXFCON, readRegByte(ERXFCON) | ERXFCON_BCEN);
    if(!temporary)
        broadcast_enabled = true;
}

void ENC28J60::disableBroadcast (bool temporary) {
    if(!temporary)
        broadcast_enabled = false;
    if(!broadcast_enabled)
        writeRegByte(ERXFCON, readRegByte(ERXFCON) & ~ERXFCON_BCEN);
}

void ENC28J60::enableMulticast () {
    writeRegByte(ERXFCON, readRegByte(ERXFCON) | ERXFCON_MCEN);
}

void ENC28J60::disableMulticast () {
    writeRegByte(ERXFCON, readRegByte(ERXFCON) & ~ERXFCON_MCEN);
}

void ENC28J60::enablePromiscuous (bool temporary) {
    writeRegByte(ERXFCON, readRegByte(ERXFCON) & ERXFCON_CRCEN);
    if(!temporary)
        promiscuous_enabled = true;
}

void ENC28J60::disablePromiscuous (bool temporary) {
    if(!temporary)
        promiscuous_enabled = false;
    if(!promiscuous_enabled) {
        writeRegByte(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);
    }
}

uint8_t ENC28J60::doBIST ( byte csPin) {
#define RANDOM_FILL     0b0000
#define ADDRESS_FILL    0b0100
#define PATTERN_SHIFT   0b1000
#define RANDOM_RACE     0b1100

// init

    selectPin = csPin;

    if (bitRead(SPCR, SPE) == 0)
        initSPI();

    pinMode(selectPin, OUTPUT);
    disableChip();

    writeOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    delay(2); // errata B7/2
    while (!(readOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_CLKRDY)) ;


    // now we can start the memory test

    uint16_t macResult;
    uint16_t bitsResult;

    // clear some of the registers registers
    writeRegByte(ECON1, 0);
    writeReg(EDMAST, 0);

    // Set up necessary pointers for the DMA to calculate over the entire memory
    writeReg(EDMAND, 0x1FFFu);
    writeReg(ERXND, 0x1FFFu);

    // Enable Test Mode and do an Address Fill
    SetBank(EBSTCON);
    writeRegByte(EBSTCON, EBSTCON_TME | EBSTCON_BISTST | ADDRESS_FILL);

    // wait for BISTST to be reset, only after that are we actually ready to
    // start the test
    // this was undocumented :(
    while (readOp(ENC28J60_READ_CTRL_REG, EBSTCON) & EBSTCON_BISTST);
    writeOp(ENC28J60_BIT_FIELD_CLR, EBSTCON, EBSTCON_TME);


    // now start the actual reading an calculating the checksum until the end is
    // reached
    writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_DMAST | ECON1_CSUMEN);
    SetBank(EDMACS);
    while(readOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_DMAST);
    macResult = readReg(EDMACS);
    bitsResult = readReg(EBSTCS);
    // Compare the results
    // 0xF807 should always be generated in Address fill mode
    if ((macResult != bitsResult) || (bitsResult != 0xF807)) {
        return 0;
    }
    // reset test flag
    writeOp(ENC28J60_BIT_FIELD_CLR, EBSTCON, EBSTCON_TME);


    // Now start the BIST with random data test, and also keep on swapping the
    // DMA/BIST memory ports.
    writeRegByte(EBSTSD, 0b10101010 | millis());
    writeRegByte(EBSTCON, EBSTCON_TME | EBSTCON_PSEL | EBSTCON_BISTST | RANDOM_FILL);


    // wait for BISTST to be reset, only after that are we actually ready to
    // start the test
    // this was undocumented :(
    while (readOp(ENC28J60_READ_CTRL_REG, EBSTCON) & EBSTCON_BISTST);
    writeOp(ENC28J60_BIT_FIELD_CLR, EBSTCON, EBSTCON_TME);


    // now start the actual reading an calculating the checksum until the end is
    // reached
    writeOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_DMAST | ECON1_CSUMEN);
    SetBank(EDMACS);
    while(readOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_DMAST);

    macResult = readReg(EDMACS);
    bitsResult = readReg(EBSTCS);
    // The checksum should be equal
    return macResult == bitsResult;
}


void ENC28J60::memcpy_to_enc(uint16_t dest, void* source, int16_t num) {
    writeReg(EWRPT, dest);
    writeBuf(num, (uint8_t*) source);
}

void ENC28J60::memcpy_from_enc(void* dest, uint16_t source, int16_t num) {
    writeReg(ERDPT, source);
    readBuf(num, (uint8_t*) dest);
}

static uint16_t endRam = ENC_HEAP_END;
uint16_t ENC28J60::enc_malloc(uint16_t size) {
    if (endRam-size >= ENC_HEAP_START) {
        endRam -= size;
        return endRam;
    }
    return 0;
}

uint16_t ENC28J60::enc_freemem() {
    return endRam-ENC_HEAP_START;
}

uint16_t ENC28J60::readPacketSlice(char* dest, int16_t maxlength, int16_t packetOffset) {
    uint16_t erxrdpt = readReg(ERXRDPT);
    int16_t packetLength;

    memcpy_from_enc((char*) &packetLength, (erxrdpt+3)%(RXSTOP_INIT+1), 2);
    packetLength -= 4; // remove crc

    int16_t bytesToCopy = packetLength - packetOffset;
    if (bytesToCopy > maxlength) bytesToCopy = maxlength;
    if (bytesToCopy <= 0) bytesToCopy = 0;

    int16_t startofSlice = (erxrdpt+7+packetOffset)%(RXSTOP_INIT+1);
    memcpy_from_enc(dest, startofSlice, bytesToCopy);
    dest[bytesToCopy] = 0;

    return bytesToCopy;
}
