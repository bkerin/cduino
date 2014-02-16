// Implementation of the interface escribed in wireless_xbee.h.

// We're using assert() to handle errors, we can't let clients turn it off.
#ifdef NDEBUG
#  error The HANDLE_ERRORS() macro in this file requires assert()
#endif
#include <assert.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>   // FIXXME: probably only needed for broken assert header
#include <string.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "uart.h"
#include "wireless_xbee.h"
#include "util.h"

// These macros are convenient during development of this module; see the
// notes about thim in wireless_xbee_test.c.
#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 3)
#undef BTRAP
#define BTRAP() BTRAP_USING (DDRD, DDD4, PORTD, PORTD4, 100.0)

void
wx_init (void)
{
  uart_init ();
}

// Define a HANDLE_ERRORS macro that either asserts the given condition,
// or simply returns depending on a compile-time setting.  We guarantee
// that this macro will evaluate its argument only once, so its safe to
// use around a function that has other effects.
#ifdef WX_ASSERT_SUCCESS
#  define HANDLE_ERRORS(condition) assert (condition)
#else
#  define HANDLE_ERRORS(condition) \
     do { \
       if ( UNLIKELY (! (condition)) ) { \
         return FALSE; \
       } \
     } while ( 0 )
#endif

static char
get_char (void)
{
  WX_WAIT_FOR_BYTE ();

  if ( WX_UART_RX_ERROR () ) {
    WX_UART_FLUSH_RX_BUFFER ();
    HANDLE_ERRORS (FALSE);   // Meaning we have an error to handle
  }

  return WX_GET_BYTE ();
}

static uint8_t
get_line (uint8_t bufsize, char *buf)
{
  // Get up to bufsize - 1 character from the serial port, or until a
  // carriage return ('\r') character is recieved, then add a terminating
  // NUL byte and return TRUE to indicate success.

  uint8_t ii;   // Index

  for ( ii = 0 ; ii < bufsize - 1 ; ii++ ) {
    buf[ii] = get_char ();
    if ( buf[ii] == '\r' ) {
      ii++;
      break;
    }
  } 

  buf[ii] = '\0';

  return TRUE;
}

uint8_t
wx_enter_at_command_mode (void)
{
  // Send the sequence which initiates AT command mode, returning true on
  // success.  After this function returns, the XBee will remain in command
  // mode for up to 10 seconds (or until exit_at_command_mode() is called.

  // This magic sequence should send us into AT command mode
  float const dwmms = 1142;   // Delay With Margin (in ms) -- AT requires 1 s
  _delay_ms (dwmms);
  WX_PUT_BYTE ('+');
  WX_PUT_BYTE ('+');
  WX_PUT_BYTE ('+');
  _delay_ms (dwmms);

  // This probably goes without saying, but we have to assert it somewhere
  assert (WX_MCOSL < UINT8_MAX); 
    
  // NOTE: this seems like more RAM than we need, but clients are eventually
  // going to need it anyway to read the (longer) responses from real query
  // commands.
  char response[WX_MCOSL];

  uint8_t sentinel = get_line (WX_MCOSL, response);
  HANDLE_ERRORS (sentinel);
  
  HANDLE_ERRORS (! strcmp (response, "OK\r"));

  return TRUE;
}

uint8_t
wx_exit_at_command_mode (void)
{
  // Require the XBee module to be in AT command mode (see
  // enter_at_command_mode()).  Leave AT command mode.

  char response[WX_MCOSL];

  HANDLE_ERRORS (wx_at_command ("CN", response));

  HANDLE_ERRORS (! strcmp (response, "OK"));

  return TRUE;
}


static void
put_command (char const *command)
{
  // Put a command out on the serial port.  The "AT" prefix and "\r" postfix
  // are automatically added to the supplied command.  This resulting bytes
  // are sent.  This routine doesn't wait for a response.

  WX_PUT_BYTE ('A');
  WX_PUT_BYTE ('T');

  uint8_t csl = strlen (command);   // Command String Length
  for ( uint8_t ii = 0 ; ii < csl ; ii++ ) {
    WX_PUT_BYTE (command[ii]);
  }

  WX_PUT_BYTE ('\r');
}

uint8_t
wx_at_command (char *command, char *output)
{
  put_command (command);
 
  HANDLE_ERRORS (get_line (WX_MCOSL, output));

  // Verify that the output string ends with '\r'
  uint8_t osl = strnlen (output, WX_MCOSL);   // Output String Length
  HANDLE_ERRORS (output[osl - 1] == '\r');

  output[osl - 1] = '\0';   // Snip the trailing "\r" as promised

  return TRUE;
}

uint8_t
wx_at_command_expect_ok (char const *command)
{
  put_command (command);

  HANDLE_ERRORS (get_char () == 'O');
  HANDLE_ERRORS (get_char () == 'K');
  HANDLE_ERRORS (get_char () == '\r');

  return TRUE;
}

uint8_t
wx_ensure_network_id_set_to (uint16_t id)
{
  char buf[WX_MCOSL];   // Buffer for command/output string storage

  uint8_t cp = sprintf_P (buf, PSTR ("ID"));   // Chars Printed
  HANDLE_ERRORS (cp == 2);  // sprintf_P gives a return value, so we check it

  HANDLE_ERRORS (wx_at_command (buf, buf));

  int const base_16 = 16;   // Base to use to convert retrieved string
  char *endptr;   //  Pointer to be set to end of converted string
  long int eidv = strtol (buf, &endptr, base_16);   // Existing ID value
  if ( *buf != '\0' && *endptr == '\0' ) {
    if ( eidv == id ) {
      return TRUE;   // ID is already set as requested, so we're done
    }
  }
  else {
    HANDLE_ERRORS (0);   // Didn't get a convertible string back from command
  }

  // Print the id into a command string (in the form expected by
  // at_command_expect_ok()).  The argument itself must be upper case,
  // without any leading "0x" or "0X".
  uint8_t const escsl = 6;   // Expected Setting Command String Length
  cp = sprintf_P (buf, PSTR ("ID%.4" PRIX16), id);
  HANDLE_ERRORS (cp == escsl);

  HANDLE_ERRORS (wx_at_command_expect_ok (buf));

  HANDLE_ERRORS (wx_at_command_expect_ok ("WR"));

  return TRUE;
}

uint8_t
wx_ensure_channel_set_to (uint8_t channel)
{
  // The channel argument must fall in the valid range.
  HANDLE_ERRORS (channel >= 0x0b);
  HANDLE_ERRORS (channel <= 0x1a);
  
  char buf[WX_MCOSL];   // Buffer for command/output string storage

  uint8_t cp = sprintf_P (buf, PSTR ("CH"));   // Chars Printed
  HANDLE_ERRORS (cp == 2);  // sprintf_P gives a return value, so we check it

  HANDLE_ERRORS (wx_at_command (buf, buf));

  int const base_16 = 16;   // Base to use to convert retrieved string
  char *endptr;   //  Pointer to be set to end of converted string
  long int echv = strtol (buf, &endptr, base_16);   // Existing CH value
  if ( *buf != '\0' && *endptr == '\0' ) {
    if ( echv == channel ) {
      return TRUE;   // CH is already set as requested, so we're done
    }
  }
  else {
    HANDLE_ERRORS (0);   // Didn't get a convertible string back from command
  }

  // Print the channel into a command string (in the form expected by
  // at_command_expect_ok()).  The argument itself must be upper case,
  // without any leading "0x" or "0X".
  uint8_t const escsl = 6;   // Expected Setting Command String Length
  cp = sprintf_P (buf, PSTR ("CH%.4" PRIX16), channel);
  HANDLE_ERRORS (cp == escsl);

  HANDLE_ERRORS (wx_at_command_expect_ok (buf));

  HANDLE_ERRORS (wx_at_command_expect_ok ("WR"));

  return TRUE;
}

uint8_t
wx_restore_defaults (void)
{
  HANDLE_ERRORS (wx_at_command_expect_ok ("RE"));

  HANDLE_ERRORS (wx_at_command_expect_ok ("WR"));
  
  return TRUE;
}

// Factor of safety to use when delaying to force radio packet transmission.
// We make this a little large since the character time might actually be
// a bit greater than what we calculate in DELAY_TO_FORCE_TRANSMISSION()
// due to start bits, padding etc.
#define PACKETIZATION_TIMEOUT_FOS 1.5

// Delay for long enough to force data already sent to the XBee to be
// transmitted.
#define DELAY_TO_FORCE_TRANSMISSION() \
  _delay_ms ( \
      BITS_PER_BYTE * \
      (1.0 / WX_BAUD) * \
      WX_TRANSPARENT_MODE_PACKETIZATION_TIMEOUT_BYTES * \
      PACKETIZATION_TIMEOUT_FOS )

// Bytes that need to be escaped when they occur in data frames
#define FRAME_DELIMITER 0x7e
#define ESCAPE          0x7d
#define XON             0x11
#define XOFF            0x13

// Bytes that need to be escaped are escaped by transmitting an ESCAPE byte
// first, then transmitting the byte value xor'ed this value.  The escaped
// byte values are recovered by xor'ing with this value again.
#define ESCAPE_MODIFIER 0x20

// The CRC algorithm being used starts with this value
#define CRC_INITIAL_VALUE 0xffff

static uint8_t
needs_escaped (uint8_t byte)
{
  // Return true iff byte is one of the ones that needs to be escaped in
  // our frame scheme.

  switch ( byte ) {
    // NOTE: we're using fall-through behavior of C switch statement here
    case FRAME_DELIMITER:
    case ESCAPE:
    case XON:
    case XOFF:
      return TRUE;
      break;
    default:
      return FALSE;
      break;
  }
}

// Put two possibly escaped CRC bytes out over the air (a total of up to 4
// real bytes).  This is probably most easily understood from its use context.
#define PUT_POSSIBLY_ESCAPED_CRC_BYTES(crc16) \
  do { \
    if ( UNLIKELY (needs_escaped (HIGH_BYTE (crc16))) ) { \
      WX_PUT_BYTE (ESCAPE); \
      WX_PUT_BYTE (HIGH_BYTE (crc16) ^ ESCAPE_MODIFIER); \
    } \
    else { \
      WX_PUT_BYTE (HIGH_BYTE (crc16)); \
    } \
    if ( UNLIKELY (needs_escaped (LOW_BYTE (crc16))) ) { \
      WX_PUT_BYTE (ESCAPE); \
      WX_PUT_BYTE (LOW_BYTE (crc16) ^ ESCAPE_MODIFIER); \
    } \
    else { \
      WX_PUT_BYTE (LOW_BYTE (crc16)); \
    } \
  } while ( 0 )

uint8_t
wx_put_frame (uint8_t count, void const *buf)
{
  uint8_t epl = 0;   // Escaped payload length
  uint8_t ii;        // Index variable

  uint16_t lcrc = CRC_INITIAL_VALUE;   // Length (and delimiter) CRC value
  uint16_t pcrc = CRC_INITIAL_VALUE;   // Payload CRC value

  // Compute checksum and escaped payload length
  for ( ii = 0 ; ii < count ; ii++ ) {
    if ( needs_escaped(((uint8_t *) buf)[ii]) ) {
      pcrc = _crc_ccitt_update (pcrc, ESCAPE);
      pcrc = _crc_ccitt_update (pcrc, ((uint8_t *) buf)[ii] ^ ESCAPE_MODIFIER);
      epl += 2;
    }
    else {
      pcrc = _crc_ccitt_update (pcrc, ((uint8_t *) buf)[ii]);
      epl++;
    }
  }

  // Compute the length field bytes and the CRC that covers the frame
  // delimiter and length bytes
  uint8_t lxorfb;   // Lenght XOR'ed Flag Byte
  uint8_t pxlb;     // Possibley Xor'ed Length Byte
  lcrc = _crc_ccitt_update (lcrc, FRAME_DELIMITER);
  if ( needs_escaped (epl) ) {
    lxorfb = WX_LENGTH_BYTE_XORED;
    pxlb = epl ^ ESCAPE_MODIFIER;
  }
  else {
    lxorfb = WX_LENGTH_BYTE_NOT_XORED;
    pxlb = epl;
  }
  lcrc = _crc_ccitt_update (lcrc, lxorfb);
  lcrc = _crc_ccitt_update (lcrc, pxlb);

  // These are constant properties of our frame format
  uint8_t const fdbc = 1;   // Frame Delimiter Byte Count
  uint8_t const lbc  = 2;   // Length Byte Count
  uint8_t const cbc  = 4;   // CRC Byte Count

  uint8_t cebc = 0;   // CRC escape byte count
  if ( needs_escaped (HIGH_BYTE (lcrc)) ) { cebc++; }
  if ( needs_escaped (LOW_BYTE (lcrc)) )  { cebc++; }
  if ( needs_escaped (HIGH_BYTE (pcrc)) ) { cebc++; }
  if ( needs_escaped (LOW_BYTE (pcrc)) )  { cebc++; }

  // If our frame won't fit in a radio packet, return FALSE
  if ( fdbc + lbc + cbc + cebc + epl > WX_TRANSPARENT_MODE_MAX_PACKET_SIZE ) {
    return FALSE;
  }

  // Transmit the actual frame
  WX_PUT_BYTE (FRAME_DELIMITER);
  WX_PUT_BYTE (lxorfb);
  WX_PUT_BYTE (pxlb);
  PUT_POSSIBLY_ESCAPED_CRC_BYTES (lcrc);
  for ( ii = 0 ; ii < count ; ii++ ) {
    // NOTE: we're using fall-through behavior of C switch statement here
    if ( needs_escaped (((uint8_t *) buf)[ii]) ) {
        WX_PUT_BYTE (ESCAPE);
        WX_PUT_BYTE (((uint8_t *) buf)[ii] ^ ESCAPE_MODIFIER);
    }
    else{
      WX_PUT_BYTE (((uint8_t *) buf)[ii]);
    }
  }
  PUT_POSSIBLY_ESCAPED_CRC_BYTES (pcrc);

  DELAY_TO_FORCE_TRANSMISSION ();

  return TRUE;
}

uint8_t
wx_put_string_frame (char const *str)
{
  assert (sizeof (char) == 1);   // Paranoid truism :)

  size_t sl = strlen (str);   // String length
  assert (sl <= UINT8_MAX);   // -1 to allow for terminating NUL

  return wx_put_frame (sl, str);
}

#define FRAME_STATE_OUTSIDE_FRAME                     1
#define FRAME_STATE_AT_LENGTH_XORED_FLAG              2
#define FRAME_STATE_AT_LENGTH_ITSELF                  3
#define FRAME_STATE_AT_LENGTH_CRC_HIGH_BYTE           4
#define FRAME_STATE_AT_LENGTH_CRC_HIGH_BYTE_ESCAPED   5
#define FRAME_STATE_AT_LENGTH_CRC_LOW_BYTE            6
#define FRAME_STATE_AT_LENGTH_CRC_LOW_BYTE_ESCAPED    7
#define FRAME_STATE_IN_PAYLOAD                        8
#define FRAME_STATE_IN_PAYLOAD_ESCAPED                9
#define FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE         10
#define FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE_ESCAPED 11 
#define FRAME_STATE_AT_PAYLOAD_CRC_LOW_BYTE          12
#define FRAME_STATE_AT_PAYLOAD_CRC_LOW_BYTE_ESCAPED  13
#define FRAME_STATE_COMPLETE                         14

// This chunk of code can be compared to the avr libc _crc_ccitt_update
// behavior.  There is a commented out block in wx_get_frame() that can be
// used to drive this function.  FIXXME: commented out sections should go
// away eventually.
/*
#define lo8(arg) ((uint8_t) (0x00ff & arg))
#define hi8(arg) ((uint8_t) ((0xff00 & arg) >> 8))
static uint16_t
crc_ccitt_update (uint16_t crc, uint8_t data)
{
  data ^= lo8 (crc);
  data ^= data << 4;
  
  return (
      (((uint16_t) data << 8) | hi8 (crc)) ^
      (uint8_t) (data >> 4) ^
      ((uint16_t) data << 3) );
}
*/

uint8_t
wx_get_frame (uint8_t mfps, uint8_t *rfps, void *buf, uint16_t timeout)
{
  uint8_t fs = FRAME_STATE_OUTSIDE_FRAME;   // Frame State
  uint16_t crc = CRC_INITIAL_VALUE;         // Cyclic Redundany Check value
  uint16_t et = 0;    // Elapsed Time
  uint8_t lxorfb = 42;   // Length XOR'ed Flag Byte (bogus initialization)
  uint8_t epl = 42;   // Escaped Payload Length  (bogus "= 42" for compiler)
  uint8_t epbr = 0;   // Escaped Payload Bytes Read (so far)

  *rfps = 0;   // We've received nothing so far

  // Commented out code useful for comparing crc_ccitt_update to the builtin
  // _crc_ccitt_update routine
  {
    /*
    uint16_t tcrc1 = CRC_INITIAL_VALUE;
    uint16_t tcrc2 = CRC_INITIAL_VALUE;
    tcrc1 = _crc_ccitt_update (tcrc1, '4');
    tcrc1 = _crc_ccitt_update (tcrc1, '2');
    tcrc1 = _crc_ccitt_update (tcrc1, '\n');
    tcrc2 = crc_ccitt_update (tcrc2, '4');
    tcrc2 = crc_ccitt_update (tcrc2, '2');
    tcrc2 = crc_ccitt_update (tcrc2, '\n');
    if ( tcrc1 == tcrc2 ) {
      CHKP_PD4 ();
    }
    if ( tcrc1 == 0xf6b4 ) {
      CHKP_PD4 ();
    }
    */
  }

  while ( et < timeout ) {

    if ( WX_BYTE_AVAILABLE () ) {
      
      if ( WX_UART_RX_ERROR () ) {
        if ( WX_UART_RX_FRAME_ERROR () ) {
          WX_UART_FLUSH_RX_BUFFER ();
          // FIXXME: could propagate error from here
        }
        // This can actually happen pretty easily if we abort early due
        // to a CRC error (bad or non-frame data) on a previous read.
        // Flushing the buffer here is sort of a weird courtesy particular
        // to this function, and shouldn't be required now given the approach
        // prescribed in the interface (see wireless_xbee.h).
        if ( WX_UART_RX_DATA_OVERRUN_ERROR () ) {
          WX_UART_FLUSH_RX_BUFFER ();
          // FIXXME: could propagate error from here
        }
        return FALSE;   // UART says somethig bad happened
      }

      uint8_t cb = WX_GET_BYTE ();   // Current Byte
          
      if ( cb == FRAME_DELIMITER ) {
        // A frame delimiter should only occur unescaped when we aren't
        // already reading a frame.  If we see it elsewhere it means corrupt
        // data.  Since this is an an error from every frame state except
        // one, we check for it just once up front.  In theory the CRC
        // check would catch this anyway since it should only occur due to
        // data corruption.  But it could also be due to a malformed frame,
        // so we check for it explicitly.
        if ( fs != FRAME_STATE_OUTSIDE_FRAME ) {
          return FALSE;
        }
      }
      // The XON and XOFF bytes should never occur unescaped in a frame
      else if ( fs != FRAME_STATE_OUTSIDE_FRAME && (cb == XON || cb == XOFF) ) {
        return FALSE;
      }
 
      switch ( fs ) {

        case FRAME_STATE_OUTSIDE_FRAME:
          if ( cb == FRAME_DELIMITER ) {
            crc = _crc_ccitt_update (crc, cb);
            fs = FRAME_STATE_AT_LENGTH_XORED_FLAG;
          }
          break;

        case FRAME_STATE_AT_LENGTH_XORED_FLAG:
          crc = _crc_ccitt_update (crc, cb);
          lxorfb = cb;
          if ( lxorfb != WX_LENGTH_BYTE_XORED &&
               lxorfb != WX_LENGTH_BYTE_NOT_XORED ) {
            return FALSE;   // This flag must be one of two possible values
          }
          fs = FRAME_STATE_AT_LENGTH_ITSELF;
          break;

        case FRAME_STATE_AT_LENGTH_ITSELF:
          crc = _crc_ccitt_update (crc, cb);
          epl = cb;
          if ( lxorfb == WX_LENGTH_BYTE_XORED ) {
            epl ^= ESCAPE_MODIFIER;
          }
          fs = FRAME_STATE_AT_LENGTH_CRC_HIGH_BYTE;
          break;

        case FRAME_STATE_AT_LENGTH_CRC_HIGH_BYTE:
          if ( cb == ESCAPE ) {
            fs = FRAME_STATE_AT_LENGTH_CRC_HIGH_BYTE_ESCAPED;
          }
          else {
            if ( cb != HIGH_BYTE (crc) ) {
              return FALSE;   // CRC of frame delimiter and length failed
            }
            fs = FRAME_STATE_AT_LENGTH_CRC_LOW_BYTE;
          }
          break;

        case FRAME_STATE_AT_LENGTH_CRC_HIGH_BYTE_ESCAPED:
          if ( (cb ^ ESCAPE_MODIFIER) != HIGH_BYTE (crc) ) {
            return FALSE;
          }
          fs = FRAME_STATE_AT_LENGTH_CRC_LOW_BYTE;
          break;
        
        case FRAME_STATE_AT_LENGTH_CRC_LOW_BYTE:
          if ( cb == ESCAPE ) {
            fs = FRAME_STATE_AT_LENGTH_CRC_LOW_BYTE_ESCAPED;
          }
          else {
            if ( cb != LOW_BYTE (crc) ) {
              return FALSE;   // CRC of frame delimiter and length failed
            }
            crc = CRC_INITIAL_VALUE;  // Reset for later use on payload
            if ( epl > 0 ) {
              fs = FRAME_STATE_IN_PAYLOAD;
            }
            else {
              fs = FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE;
            }
          }
          break;
        
        case FRAME_STATE_AT_LENGTH_CRC_LOW_BYTE_ESCAPED:
          if ( (cb ^ ESCAPE_MODIFIER) != LOW_BYTE (crc) ) {
            return FALSE;
          }
          crc = CRC_INITIAL_VALUE;  // Reset for later use on payload
          if ( epl > 0 ) {
            fs = FRAME_STATE_IN_PAYLOAD;
          }
          else {
            fs = FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE;
          }
          break;

        case FRAME_STATE_IN_PAYLOAD:
          crc = _crc_ccitt_update (crc, cb);
          if ( cb == ESCAPE ) {
            fs = FRAME_STATE_IN_PAYLOAD_ESCAPED;
          } 
          else {
            ((uint8_t *) buf)[*rfps] = cb;
            (*rfps)++;
          }
          epbr++;
          if ( epbr == epl ) {
            fs = FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE;
          }
          else if ( *rfps == mfps ) {
            return FALSE;   // Frame exceeded caller-supplied max size
          }
          break;

        case FRAME_STATE_IN_PAYLOAD_ESCAPED:
          crc = _crc_ccitt_update (crc, cb);
          ((uint8_t *) buf)[*rfps] = cb ^ ESCAPE_MODIFIER;
          (*rfps)++;
          epbr++;
          if ( epbr == epl ) {
            fs = FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE;
          }
          // FIXXME: we could detect this error once we get as far as
          // reading the length in the frame, but wasting a little time
          // reading the frame bytes probably doesn't make much difference
          // at least given our current very coarse error reporting scheme
          else if ( *rfps == mfps ) {
            return FALSE;   // Frame exceeded caller-supplied max size
          }
          else {
            fs = FRAME_STATE_IN_PAYLOAD;
          }
          break;

        case FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE:
          if ( cb == ESCAPE ) {
            fs = FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE_ESCAPED;
          }
          else {
            if ( cb != HIGH_BYTE (crc) ) {
              return FALSE;   // CRC failed
            }
            fs = FRAME_STATE_AT_PAYLOAD_CRC_LOW_BYTE;
          }
          break;

        case FRAME_STATE_AT_PAYLOAD_CRC_HIGH_BYTE_ESCAPED:
          if ( (cb ^ ESCAPE_MODIFIER) != HIGH_BYTE (crc) ) {
            return FALSE;
          }
          fs = FRAME_STATE_AT_PAYLOAD_CRC_LOW_BYTE;
          break;

        case FRAME_STATE_AT_PAYLOAD_CRC_LOW_BYTE:
          if ( cb == ESCAPE ) {
            fs = FRAME_STATE_AT_PAYLOAD_CRC_LOW_BYTE_ESCAPED;
          }
          else {
            if ( cb != LOW_BYTE (crc) ) {
              return FALSE;   // CRC failed
            }
            // Frame is complete and correct.  Note that we don't ever need
            // to actually set fs to FRAME_STATE_COMPLETE.
            return TRUE;  
          }
          break;

        case FRAME_STATE_AT_PAYLOAD_CRC_LOW_BYTE_ESCAPED:
          if ( (cb ^ ESCAPE_MODIFIER) != LOW_BYTE (crc) ) {
            return FALSE;
          }
          // Frame is complete and correct.  Note that we don't ever need
          // to actually set fs to FRAME_STATE_COMPLETE.
          return TRUE;  

        default:
          assert (0);   // Shouldn't be here
          break;
      }
    }

    else {
      // If there isn't any data ready to read, wait a little bit and
      // try again.  1 ms is reasonably here since its about the time the
      // serial port takes to receive a character at 9600 baud (see the BUGS
      // AND POTENTIAL WORK-AROUNDS section of the POD text in the usb_xbee
      // perl script for the timing calculations).  Not that it matters much,
      // since its a busy wait anyway.
      uint16_t const poll_interval_ms = 1;
      _delay_ms (poll_interval_ms);
      et += poll_interval_ms;    // Elapsed Time
    }

  }

  return FALSE;   // Timeout
}

uint8_t
wx_get_string_frame (uint8_t msl, char *str, uint16_t timeout)
{
  uint8_t bytes_received;
  uint8_t sentinel = wx_get_frame (msl + 1, &bytes_received, str, timeout);
  if ( ! sentinel ) {
    return FALSE;
  }
  if ( str[bytes_received - 1] != '\0' ) {
    if ( bytes_received == msl ) {
      return FALSE;   // Client hasn't provided a big enough buffer str
    }
    str[bytes_received] = '\0';
  }
  
  return TRUE;
}
