/** @file */

#ifndef BufferFiller_h
#define BufferFiller_h

#include "EtherCard.h"


/** This class populates network send and receive buffers.
*
*   This class provides formatted printing into memory. Users can use it to write into send buffers.
*
*   Nota: PGM_P: is a pointer to a string in program space (defined in the source code, updated to PROGMEM)
*
*   # Format string
*
*   | Format | Parameter   | Output
*   |--------|-------------|----------
*   | $D     | uint16_t    | Decimal representation
*   | $T ¤   | double      | Decimal representation with 3 digits after decimal sign ([-]d.ddd)
*   | $H     | uint16_t    | Hexadecimal value of lsb (from 00 to ff)
*   | $L     | long        | Decimal representation
*   | $S     | const char* | Copy null terminated string from main memory
*   | $F     | PGM_P       | Copy null terminated string from program space
*   | $E     | byte*       | Copy null terminated string from EEPROM space
*   | $$     | _none_      | '$'
*
*   ¤ _Available only if FLOATEMIT is defined_
*
*   # Examples
*   ~~~~~~~~~~~~~{.c}
*     uint16_t ddd = 123;
*     double ttt = 1.23;
*     uint16_t hhh = 0xa4;
*     long lll = 123456789;
*     char * sss;
*     char fff[] PROGMEM = "MyMemory";
*
*     sss[0] = 'G';
*     sss[1] = 'P';
*     sss[2] = 'L';
*     sss[3] = 0;
*     buf.emit_p( PSTR("ddd=$D\n"), ddd );  // "ddd=123\n"
*     buf.emit_p( PSTR("ttt=$T\n"), ttt );  // "ttt=1.23\n" **TO CHECK**
*     buf.emit_p( PSTR("hhh=$H\n"), hhh );  // "hhh=a4\n"
*     buf.emit_p( PSTR("lll=$L\n"), lll );  // "lll=123456789\n"
*     buf.emit_p( PSTR("sss=$S\n"), sss );  // "sss=GPL\n"
*     buf.emit_p( PSTR("fff=$F\n"), fff );  // "fff=MyMemory\n"
*   ~~~~~~~~~~~~~
*
*/
class BufferFiller : public Print {
    uint8_t *start; //!< Pointer to start of buffer
    uint8_t *ptr; //!< Pointer to cursor position
public:
    /** @brief  Empty constructor
    */
    BufferFiller () {}

    /** @brief  Constructor
    *   @param  buf Pointer to the ethernet data buffer
    */
    BufferFiller (uint8_t* buf) : start (buf), ptr (buf) {}

    /** @brief  Add formatted text to buffer
    *   @param  fmt Format string (see Class description)
    *   @param  ... parameters for format string
    */
    void emit_p (const char* fmt PROGMEM, ...);

    /** @brief  Add data to buffer from main memory
    *   @param  s Pointer to data
    *   @param  n Number of characters to copy
    */
    void emit_raw (const char* s, uint16_t n) { memcpy(ptr, s, n); ptr += n; }

    /** @brief  Add data to buffer from program space string
    *   @param  p Program space string pointer
    *   @param  n Number of characters to copy
    */
    void emit_raw_p (const char* p PROGMEM, uint16_t n) { memcpy_P(ptr, p, n); ptr += n; }

    /** @brief  Get pointer to start of buffer
    *   @return <i>uint8_t*</i> Pointer to start of buffer
    */
    uint8_t* buffer () const { return start; }

    /** @brief  Get cursor position
    *   @return <i>uint16_t</i> Cursor position
    */
    uint16_t position () const { return ptr - start; }

    /** @brief  Write one byte to buffer
    *   @param  v Byte to add to buffer
    */
    virtual WRITE_RESULT write (uint8_t v) { *ptr++ = v; WRITE_RETURN }
};

#endif
