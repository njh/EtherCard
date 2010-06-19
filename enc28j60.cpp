// Microchip ENC28J60 Ethernet Interface Driver
// Author: Guido Socher
// Copyright: GPL V2
// 
// Based on the enc28j60.c file from the AVRlib library by Pascal Stang.
// For AVRlib See http://www.procyonengineering.com/
// Used with explicit permission of Pascal Stang.
//
// Mods bij jcw, 2010-05-20

#include "enc28j60.h"
#include "enc28j60_defs.h"
#include <WProgram.h>

static byte Enc28j60Bank;
static int16_t gNextPacketPtr;

//AT: Use pin 10 for nuelectronics.com compatible ethershield.
#define ENC28J60_CONTROL_CS 8

void ENC28J60::spiInit() {
    const byte SPI_SS   = 10;
    const byte SPI_MOSI	= 11;
    const byte SPI_MISO	= 12;
    const byte SPI_SCK	= 13;
    
    pinMode(SPI_SS, OUTPUT);
    pinMode(SPI_MOSI, OUTPUT);
	pinMode(SPI_SCK, OUTPUT);	
	pinMode(SPI_MISO, INPUT);
	
	digitalWrite(SPI_MOSI, HIGH);
	digitalWrite(SPI_MOSI, LOW);
	digitalWrite(SPI_SCK, LOW);

    SPCR = (1<<SPE)|(1<<MSTR);
    // SPCR = (1<<SPE)|(1<<MSTR) | (1<<SPR0);
	SPSR |= (1<<SPI2X);
}

static void enableChip() {
    cli();
#if ENC28J60_CONTROL_CS == 8
    bitClear(PORTB, 0); // much faster
#else
    digitalWrite(ENC28J60_CONTROL_CS, LOW);
#endif
}

static void disableChip() {
#if ENC28J60_CONTROL_CS == 8
    bitSet(PORTB, 0); // much faster
#else
    digitalWrite(ENC28J60_CONTROL_CS, HIGH);
#endif
    sei();
}

static void sendSPI(byte data) {
    SPDR = data;
    while (!(SPSR&(1<<SPIF)))
        ;
}

static byte ReadOp(byte op, byte address) {
    enableChip();
    sendSPI(op | (address & ADDR_MASK));
    sendSPI(0x00);
    if (address & 0x80)
        sendSPI(0x00);
    byte result = SPDR;
    disableChip();
    return result;
}

static void WriteOp(byte op, byte address, byte data) {
    enableChip();
    sendSPI(op | (address & ADDR_MASK));
    sendSPI(data);
    disableChip();
}

static void ReadBuffer(word len, byte* data) {
    enableChip();
    sendSPI(ENC28J60_READ_BUF_MEM);
    while (len--) {
        sendSPI(0x00);
        *data++ = SPDR;
    }
    disableChip();
    *data='\0';
}

static word ReadBufferWord() {
    word result;
    ReadBuffer(2, (byte*) &result);
    return result;
}

static void WriteBuffer(word len, byte* data) {
    enableChip();
    sendSPI(ENC28J60_WRITE_BUF_MEM);
    while (len--)
        sendSPI(*data++);
    disableChip();
}

static void SetBank(byte address) {
    if ((address & BANK_MASK) != Enc28j60Bank) {
        WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_BSEL1|ECON1_BSEL0);
        Enc28j60Bank = address & BANK_MASK;
        WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, Enc28j60Bank>>5);
    }
}

static byte ReadByte(byte address) {
    SetBank(address);
    return ReadOp(ENC28J60_READ_CTRL_REG, address);
}

static void WriteByte(byte address, byte data) {
    SetBank(address);
    WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

static void WriteWord(byte address, word data) {
    WriteByte(address, data);
    WriteByte(address + 1, data >> 8);
}

static word PhyReadHi(byte address) {
    WriteByte(MIREGADR, address);
    WriteByte(MICMD, MICMD_MIIRD);
    while (ReadByte(MISTAT) & MISTAT_BUSY)
        ;
    WriteByte(MICMD, 0x00);
    return ReadByte(MIRDH);
}

static void PhyWriteWord(byte address, word data) {
    WriteByte(MIREGADR, address);
    WriteWord(MIWRL, data);
    while (ReadByte(MISTAT) & MISTAT_BUSY)
        ;
}

byte enc28j60Init(byte* macaddr) {
    pinMode(ENC28J60_CONTROL_CS, OUTPUT);
    disableChip();
    
    WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    while (!ReadOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_CLKRDY)
        ;
        
    gNextPacketPtr = RXSTART_INIT;
    WriteWord(ERXSTL, RXSTART_INIT);
    WriteWord(ERXRDPTL, RXSTART_INIT);
    WriteWord(ERXNDL, RXSTOP_INIT);
    WriteWord(ETXSTL, TXSTART_INIT);
    WriteWord(ETXNDL, TXSTOP_INIT);
    WriteByte(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
    WriteWord(EPMM0, 0x303f);
    WriteWord(EPMCSL, 0xf7f9);
    WriteByte(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
    WriteByte(MACON2, 0x00);
    WriteOp(ENC28J60_BIT_FIELD_SET, MACON3,
                        MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
    WriteWord(MAIPGL, 0x0C12);
    WriteByte(MABBIPG, 0x12);
    WriteWord(MAMXFLL, MAX_FRAMELEN);  
    WriteByte(MAADR5, macaddr[0]);
    WriteByte(MAADR4, macaddr[1]);
    WriteByte(MAADR3, macaddr[2]);
    WriteByte(MAADR2, macaddr[3]);
    WriteByte(MAADR1, macaddr[4]);
    WriteByte(MAADR0, macaddr[5]);
    PhyWriteWord(PHCON2, PHCON2_HDLDIS);
    SetBank(ECON1);
    WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
    WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

    return ReadByte(EREVID);
}

byte enc28j60linkup(void) {
    return (PhyReadHi(PHSTAT2) >> 2) & 1;
}

void enc28j60PacketSend(word len, byte* packet) {
    while (ReadOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
        if ( (ReadByte(EIR) & EIR_TXERIF) ) {
            WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
            WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
        }
    WriteWord(EWRPTL, TXSTART_INIT);
    WriteWord(ETXNDL, TXSTART_INIT+len);
    WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
    WriteBuffer(len, packet);
    WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

byte enc28j60hasRxPkt(void) {
    return ReadByte(EPKTCNT) > 0;
}

word enc28j60PacketReceive(word maxlen, byte* packet) {
    word len = 0;
    if (ReadByte(EPKTCNT) > 0) {
        WriteWord(ERDPTL, gNextPacketPtr);
        gNextPacketPtr  = ReadBufferWord();
        len = ReadBufferWord() - 4; //remove the CRC count
        if (len>maxlen-1)
            len=maxlen-1;
        word rxstat  = ReadBufferWord();
        if ((rxstat & 0x80)==0)
            len=0;
        else
            ReadBuffer(len, packet);
        if (gNextPacketPtr - 1 > RXSTOP_INIT)
            WriteWord(ERXRDPTL, RXSTOP_INIT);
        else
            WriteWord(ERXRDPTL, gNextPacketPtr - 1);
        WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
    }
    return len;
}
