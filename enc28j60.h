// Microchip ENC28J60 Ethernet Interface Driver
// Author: Pascal Stang
// Modified by: Guido Socher
// Copyright: GPL V2
//
// This driver provides initialization and transmit/receive
// functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
// chip, using an SPI interface to the host processor.
//
// 2010-05-20 <jc@wippler.nl>
/** @file */

#ifndef ENC28J60_H
#define ENC28J60_H

// buffer boundaries applied to internal 8K ram
// the entire available packet buffer space is allocated

#define RXSTART_INIT        0x0000  // start of RX buffer, (must be zero, Rev. B4 Errata point 5)
#define RXSTOP_INIT         0x0BFF  // end of RX buffer, room for 2 packets
 
#define TXSTART_INIT        0x0C00  // start of TX buffer, room for 1 packet
#define TXSTOP_INIT         0x11FF  // end of TX buffer

#define SCRATCH_START       0x1200  // start of scratch area
#define SCRATCH_LIMIT       0x2000  // past end of area, i.e. 3 Kb
#define SCRATCH_PAGE_SHIFT  6       // addressing is in pages of 64 bytes
#define SCRATCH_PAGE_SIZE   (1 << SCRATCH_PAGE_SHIFT)
#define SCRATCH_PAGE_NUM    ((SCRATCH_LIMIT-SCRATCH_START) >> SCRATCH_PAGE_SHIFT)
#define SCRATCH_MAP_SIZE    (((SCRATCH_PAGE_NUM % 8) == 0) ? (SCRATCH_PAGE_NUM / 8) : (SCRATCH_PAGE_NUM/8+1))

// area in the enc memory that can be used via enc_malloc; by default 0 bytes; decrease SCRATCH_LIMIT in order
// to use this functionality
#define ENC_HEAP_START      SCRATCH_LIMIT
#define ENC_HEAP_END        0x2000

/** This class provide low-level interfacing with the ENC28J60 network interface. This is used by the EtherCard class and not intended for use by (normal) end users. */
class ENC28J60 {
public:
    static uint8_t buffer[]; //!< Data buffer (shared by receive and transmit)
    static uint16_t bufferSize; //!< Size of data buffer
    static bool broadcast_enabled; //!< True if broadcasts enabled (used to allow temporary disable of broadcast for DHCP or other internal functions)
    static bool promiscuous_enabled; //!< True if promiscuous mode enabled (used to allow temporary disable of promiscuous mode)

    static uint8_t* tcpOffset () { return buffer + 0x36; } //!< Pointer to the start of TCP payload

    /**   @brief  Initialise SPI interface
    *     @note   Configures Arduino pins as input / output, etc.
    */
    static void initSPI ();

    /**   @brief  Initialise network interface
    *     @param  size Size of data buffer
    *     @param  macaddr Pointer to 6 byte hardware (MAC) address
    *     @param  csPin Arduino pin used for chip select (enable network interface SPI bus). Default = 8
    *     @return <i>uint8_t</i> ENC28J60 firmware version or zero on failure.
    */
    static uint8_t initialize (const uint16_t size, const uint8_t* macaddr,
                               uint8_t csPin = 8);

    /**   @brief  Check if network link is connected
    *     @return <i>bool</i> True if link is up
    */
    static bool isLinkUp ();

    /**   @brief  Sends data to network interface
    *     @param  len Size of data to send
    *     @note   Data buffer is shared by receive and transmit functions
    */
    static void packetSend (uint16_t len);

    /**   @brief  Copy received packets to data buffer
    *     @return <i>uint16_t</i> Size of received data
    *     @note   Data buffer is shared by receive and transmit functions
    */
    static uint16_t packetReceive ();

    /**   @brief  Copy data from ENC28J60 memory
    *     @param  page Data page of memory
    *     @param  data Pointer to buffer to copy data to
    */
    static void copyout (uint8_t page, const uint8_t* data);

    /**   @brief  Copy data to ENC28J60 memory
    *     @param  page Data page of memory
    *     @param  data Pointer to buffer to copy data from
    */
    static void copyin (uint8_t page, uint8_t* data);

    /**   @brief  Get single byte of data from ENC28J60 memory
    *     @param  page Data page of memory
    *     @param  off Offset of data within page
    *     @return Data value
    */
    static uint8_t peekin (uint8_t page, uint8_t off);

    /**   @brief  Put ENC28J60 in sleep mode
    */
    static void powerDown();  // contrib by Alex M.

    /**   @brief  Wake ENC28J60 from sleep mode
    */
    static void powerUp();    // contrib by Alex M.

    /**   @brief  Enable reception of broadcast messages
    *     @param  temporary Set true to temporarily enable broadcast
    *     @note   This will increase load on received data handling
    */
    static void enableBroadcast(bool temporary = false);

    /**   @brief  Disable reception of broadcast messages
    *     @param  temporary Set true to only disable if temporarily enabled
    *     @note   This will reduce load on received data handling
    */
    static void disableBroadcast(bool temporary = false);

    /**   @brief  Enables reception of multicast messages
    *     @note   This will increase load on received data handling
    */
    static void enableMulticast ();
    
    /**   @brief  Enables reception of all messages
    *     @param  temporary Set true to temporarily enable promiscuous
    *     @note   This will increase load significantly on received data handling
    *     @note   All messages will be accepted, even messages with destination MAC other than own
    *     @note   Messages with invalid CRC checksum will still be rejected
    */
    static void enablePromiscuous (bool temporary = false);
    
    /**   @brief  Disable reception of all messages and go back to default mode
    *     @param  temporary Set true to only disable if temporarily enabled
    *     @note   This will reduce load on received data handling
    *     @note   In this mode only unicast and broadcast messages will be received
    */
    static void disablePromiscuous(bool temporary = false);

    /**   @brief  Disable reception of multicast messages
    *     @note   This will reduce load on received data handling
    */
    static void disableMulticast();

    /**   @brief  Reset and fully initialise ENC28J60
    *     @param  csPin Arduino pin used for chip select (enable SPI bus)
    *     @return <i>uint8_t</i> 0 on failure
    */
    static uint8_t doBIST(uint8_t csPin = 8);

    /**   @brief  Copies a slice from the current packet to RAM
    *     @param  dest pointer in RAM where the data is copied to
    *     @param  maxlength how many bytes to copy; 
    *     @param  packetOffset where within the packet to start; if less than maxlength bytes are available only the remaining bytes are copied.
    *     @return <i>uint16_t</i> the number of bytes that have been read
    *     @note   At the destination at least maxlength+1 bytes should be reserved because the copied content will be 0-terminated.
    */                   
    static uint16_t readPacketSlice(char* dest, int16_t maxlength, int16_t packetOffset);

    /** @brief  reserves a block of RAM in the memory of the enc chip
     *  @param  size number of bytes to reserve
     *  @return <i>uint16_t</i> start address of the block within the enc memory. 0 if the remaining memory for malloc operation is less than size.   
     *  @note  There is no enc_free(), i.e., reserved blocks stay reserved for the duration of the program. 
     *  @note  The total memory available for malloc-operations is determined by ENC_HEAP_END-ENC_HEAP_START, defined in enc28j60.h; by default this is 0, i.e., you have to change these values in order to use enc_malloc().  
     */
    static uint16_t enc_malloc(uint16_t size);

    /** @brief  returns the amount of memory within the enc28j60 chip that is still available for malloc.
     *  @return <i>uint16_t</i> the amount of memory in bytes.
     */
    static uint16_t enc_freemem();

    /** @brief copies a block of data from SRAM to the enc memory
        @param dest destination address within enc memory
        @param source source pointer to a block of SRAM in the arduino
        @param num number of bytes to copy
        @note  There is no sanity check. Handle with care 
     */
    static void memcpy_to_enc(uint16_t dest, void* source, int16_t num);

     /** @brief copies a block of data from the enc memory to SRAM
        @param dest destination address within SRAM
        @param source source address within enc memory
        @param num number of bytes to copy
     */
    static void memcpy_from_enc(void* dest, uint16_t source, int16_t num);
};

typedef ENC28J60 Ethernet; //!< Define alias Ethernet for ENC28J60


/** Workaround for Errata 13.
*   The transmission hardware may drop some packets because it thinks a late collision
*   occurred (which should never happen if all cable length etc. are ok). If setting
*   this to 1 these packages will be retried a fixed number of times. Costs about 150bytes
*   of flash.
*/
#define ETHERCARD_RETRY_LATECOLLISIONS 0

/** Enable pipelining of packet transmissions.
*   If enabled the packetSend function will not block/wait until the packet is actually
*   transmitted; but instead this wait is shifted to the next time that packetSend is
*   called. This gives higher performance; however in combination with 
*   ETHERCARD_RETRY_LATECOLLISIONS this may lead to problems because a packet whose
*   transmission fails because the ENC-chip thinks that it is a late collision will
*   not be retried until the next call to packetSend.
*/
#define ETHERCARD_SEND_PIPELINING 0
#endif
