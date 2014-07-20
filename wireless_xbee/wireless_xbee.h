// MaxStream XBee Series 1 (aka XBee 802.15.4) Wireless Module Interface
//
// Test driver: wireless_xbee_test.c    Implementation: wireless_xbee.c
//
// You really want to read this entire interface file, and maybe the
// referenced material as well.  There are many ways to go wrong.
//
// This module uses the ATmega328P hardware serial port to communicate
// with the XBee.  It features high-level support for a few configuration
// parameters that people are most likely to desire to change, some low-level
// functions for people who need to do more extensive XBee reconfiguration,
// some data Tx and Rx macros which just use the underlying serial interface,
// and a higher-level Tx/Rx interface featuring atomic data frames.
//
// Though this module should not be dependent on any particular shield,
// the Sparkfun XBee Shield (Sparkfun part number WRL-10854) was used for
// development.  Its available on its own, or as part of the Sparkfun "XBee
// Wireless Kit Retail" (Sparkfun part number RTL-11445), which also includes
// the actual XBee modules and a stand-alone miniature USB XBee interface
// board known as an "XBee Explorer USB" (Sparkfun part number WRL-08687)
// as well.  This last is a must-have for development IMO.  Make sure to get
// a USB Type A to USB Mini-B cable as well, it isn't included in the kit.
// Alternatively (if you aren't getting the whole kit) you can grab an XBee
// Explorer Dongle (Sparkfun part number WRL-09819), then you don't need
// the cable.  There is another different Sparkfun USB dongle that didn't
// work for me: see the paragraph on the WRL-09819 in usb_xbee for details.
//
// Its important to realize that pushing the reset button on the XBee shield
// only resets the Arduino, not the XBee itself.  Same with reprogramming
// the Arduino.  Its possible to wedge the XBee. If things work the first
// time through but not on subsequent attempts, you may need to power
// everything down (or run a line to the XBee RESET input as described
// below and make your program reset the XBee on startup).
//
// Its possible to use an XBee shield without using the XBee SLEEP_RQ
// or RESET or signals, but in battery powered designs at least you'll
// want to use both.  SLEEP_RQ lets you save power, and RESET is useful
// for ensuring that the XBee always gets reset whenever the ATMega does.
// Unfortunately the sparkfun shield at least doesn't break these XBee lines
// out anywhere, but you can make your own strange wiring to the chip pins
// (or perhaps make your own Arduino-free board :).  This interface supports
// the use of these lines use via two macros: WX_SLEEP_RQ_CONTROL_PIN
// and WX_RESET_CONTROL_PIN.  Clients can define these before including
// this header to enable some other macros for putting the XBee to sleep
// and resetting it.  If you use either, you must use both (IIRC because
// I think I saw the XBee failing to reset when asleep, so an implicit
// wake-up is required).  The wireless_xbee_test.c file has a (commented
// out) snippet at the start of its main() function showing the whole XBee
// initialization procedure when these signals are being used.
//
// The directory for this module contains a perl script called usb_xbee that
// can be used to configure or send/receive data to/from an XBee Explorer
// or XBee USB dongle.  You can view its documentation using
//
//    pod2text usb_xbee  | less
//
// or so.  The test driver in wireless_xbee_test.c depends on this script
// for some of its testing.  The list of tested XBee modules given in the
// usb_xbee documentation applies to this interface as well.
//
// Sparkfun has IMO the best information page for XBee modules:
//
//   https://www.sparkfun.com/pages/xbee_guide
//
// There are a couple pages on the Arduino site that are worth reading,
// particularly if you need to do more extensive XBee configuration than
// what this interface provides directly.  WARNING: read the comment near
// the DEFAULT_CHANNEL_STRING define in wireless_xbee_test.c for an important
// caveat though.
//
//   http://arduino.cc/en/Main/ArduinoWirelessShield
//   http://arduino.cc/en/Guide/ArduinoWirelessShield
//
// Because this module uses the hardware serial port to communicate with
// the XBee, the edit-compile-debug process is easier if you use in-system
// programming for upload, rather than the serial port.  There are some clues
// about how to do this near the CHKP_PD4() macro in wireless_xbee_test.c
// Otherwise, make sure to take note of the tiny switch on the WRL-10854
// XBee Shield: it needs to be in the DLINE position for serial programming
// to work, and the UART position for communication between the Arduino
// and the XBee to work.  So you'll end up toggling the switch twice and
// pushing the reset button once per edit-compile-debug cycle.  I believe
// the same goes for many other XBee shields, including the official
// Arduino one, though it gives the switch positions different names (see
// http://arduino.cc/en/Main/ArduinoWirelessShield).
//
// At least for the Sparkfun shield, when the switch is in the DLINE
// position, the data input and output signals (DOUT and DIN) of the XBee
// end up connected (through a level shifter) to the Digital 2 and Digital 3
// Arduino pins (PD2 and PD3 on the ATMega328P).  This isn't useful for this
// library, since it doesn't support over-the-air programming of the Arduino.
// But of course it can screw things up if you're trying to use those pins
// for some other purpose, so its something to be aware of.
//
// This module doesn't do anything with the DTR/RTS lines of the XBee.
// Sending data too fast can overwhelm the XBee.  Its always possible to
// send an entire frame without causing any overflow though (assuming the
// queue was clear to start with).  See the XBee datasheet for details.

#ifndef WIRELESS_XBEE_H
#define WIRELESS_XBEE_H

#include <inttypes.h>

#include "dio.h"
#include "uart.h"


///////////////////////////////////////////////////////////////////////////////
//
// About Errror Handling
//
// This module really doesn't do much of it.  It just returns true on
// succes, and false otherwise.  If the macro WX_ASSERT_SUCCESS is defined
// it mostly doesn't even do that: it simply calls assert() internally
// if something fails.  In this case, all the function descriptions which
// indicate sentinel return values are wrong unless otherwise noted.
//
// For all the AT command mode functions, a false result almost certainly
// means something isn't set up right and you're not talking to the XBee
// at all, or else there's a bug.  For more details about where exactly
// things are failing, you'll need to instrument the source code for this
// module with CHKP() or CHKP_PD4() or something similar.
//
// It *might* be worth retrying some functions in some cases on account of
// noise or traffic.  Maybe.  But I don't know when exactly.
//
// Note that the actual over-the-air transmission (normally resulting from
// WX_PUT_BYTE() or one of its callers) does not by itself involve any
// feedback at all about whether the transmission was actually received
// anywhere.  In the default point-to-multipoint XBee configuration, all
// nearby modules with the same network ID (see wx_ensure_network_id_set_to())
// and channel (see wx_ensure_channel_set_to()) will hopefully receive
// the transmission, but its up to you to arrange for them to send back
// something saying they have if you really want to know.  No radio system
// is entirely immune to noise.  Also, in the default configuration the
// RF data rate is greater than the serial interface data rate, and all
// nodes recieve any transmission (point-to-multipoint), so if many nodes
// decide to talk at once the receiving buffers will likely overflow and
// some transmitted data will fail to make its way via the serial port out
// of the receiving XBee module(s).
//

// Serial communication rate at which we talk to the XBee.  Because our
// underlying serial module always communicates at this rate, this value isn't
// easy to change.
#define WX_BAUD 9600

// Clients can define this macro to an IO pin from dio.h (e.g. DIO_PIN_PB1)
// before including this header to enable some macros to control explicit
// XBee sleep requests via the XBee SLEEP_RQ pin.  For this to work, the
// XBee must be configured to use a sleep mode that honors the SLEEP_RQ pin
// (e.g. with wx_at_command_expect_ok("SM1")).  See also the general notes
// above where this macro is mentioned.
#ifdef WX_SLEEP_RQ_CONTROL_PIN

   // Some pins we set as input without pullups a lot of the time to be
   // sure they don't waste power, but not the sleep request line that's
   // responsible for putting the XBee to sleep.  We want that configured
   // as an output always.  We may not really need to delay after setting
   // the line (using WX_WAKE() for convenience), but its a conservative
   // thing to do.
#  define WX_SLEEP_RQ_CONTROL_PIN_INIT() \
     do { \
       DIO_INIT (WX_SLEEP_RQ_CONTROL_PIN, DIO_OUTPUT, DIO_DONT_CARE, LOW); \
       WX_WAKE (); \
     } while ( 0 )

   // Ensure that the XBee is set on the path towards sleep.  It finishes
   // up housekeeping before it goes to sleep.
#  define WX_SLEEP() DIO_SET_HIGH (WX_SLEEP_RQ_CONTROL_PIN)

   // Wake the XBee from sleep.
   //
   // WARNING: it takes the XBee some time to wake up.  The XBee datasheet
   // says that the module needs 13.2 ms to wake from hibernate (XBee
   // configuration parameter SM=1).  It also says the XBee will by ready
   // for transmission two 'byte times' after it takes its CTS line low.
   // This interface doesn't require the CTS line to be monitored, and
   // transmissions do indeed get scrambled up if you rush things, so we
   // give it a full 20 ms.  XBee doze mode (SM=2) is worth considering if
   // you need faster wake-up (at the cost of more power of course).
#  define WX_WAKE() \
     do { \
     DIO_SET_LOW (WX_SLEEP_RQ_CONTROL_PIN); \
     float XxX_wakeup_time_ms = 20.0; \
     _delay_ms (XxX_wakeup_time_ms); \
   } while ( 0 )

#endif

// Clients can define this macro to an IO pin from dio.h (e.g. DIO_PIN_PD6)
// before including this header to enable a macro to reset the XBee module
// using its RESET line.  The WX_SLEEP_RQ_CONTROL_PIN must also be defined
// if this macro is defined.  See also the general notes above where this
// macro is mentioned.
#ifdef WX_RESET_CONTROL_PIN

   // If the RESET pin is going to be used, we require the SLEEP_RQ pin
   // to be set up as well.  If I recall correctly, this is because RESET
   // doesn't work when the device is sleeping, so we want to do an implict
   // wake before resetting.
#  ifndef WX_SLEEP_RQ_CONTROL_PIN
#    error WX_RESET_CONTROL_PIN is defined but WX_SLEEP_RQ_CONTROL_PIN is not
#  endif

   // WARNING: WX_SLEEP_RQ_CONTROL_PIN_INIT() must be called before using
   // this macro.  Reset the XBee.  I don't actually know how long it takes
   // to boot up because the datasheet doesn't do a good job of saying,
   // so we give it plenty of time.  We reconfigure the control pin as an
   // input when we're not using it out of paranoia about power waste (this
   // is why there is no seperate macro to initialized the reset control pin).
#  define WX_RESET() \
     do { \
       WX_WAKE (); \
       DIO_INIT (WX_RESET_CONTROL_PIN, DIO_OUTPUT, DIO_DONT_CARE, LOW); \
       float XxX_reset_hold_time_us = 142.0; \
       _delay_us (XxX_reset_hold_time_us); \
       DIO_INIT ( \
         WX_RESET_CONTROL_PIN, \
         DIO_INPUT, \
         DIO_DISABLE_PULLUP, \
         DIO_DONT_CARE ); \
      float XxX_reboot_time_ms = 42.0; \
      _delay_ms (XxX_reboot_time_ms); \
    } while ( 0 )

#endif

// Initialize the interface to the XBee.  Currently this interface only
// supports talking to XBee devices over the hardware serial port at WX_BAUD
// Baud, with eight data bits, no parity, and one stop bit (8-N-1 format).
// So the serial port is initialized with those parameters, and that's
// all this routine does.  Note that this routine doesn't use the XBee
// RESET line at all.  You aren't even required to have a connection to
// that line.  However, if you do have it connected (see the reference to
// WX_RESET_CONTROL_PIN above), you likely want use the WX_RESET() macro
// before calling this function.  The ATMega328P datasheet says that USART0
// must be reinitialized after waking from sleep.  In practive I haven't
// found it to need this, but this function is guaranteed to be callable
// in this situation just in case (it will reinitialize USART0).
void
wx_init (void);

// Uncomment this or otherwise arrange for this macro to be defined to
// enable simplified error handling.  See "About Error Handling" above.
//#define WX_ASSERT_SUCCESS

// All functions that require an AT command to be executed (except
// wx_enter_at_command_mode()) will fail if they don't get a complete
// response within about this amount of time after sending the request.
// They might fail more quickly.  WARNING: for at least one XBee command,
// ED (energy scan), this won't be long enough, and you may need to define
// a higher limit yourself before including this header, or propagate the
// timeout mechanics in the implementation in wireless_xbee.c up to the
// interface level.
#ifndef WX_AT_COMMAND_RESPONSE_TIME_LIMIE_MS
#  define WX_AT_COMMAND_RESPONSE_TIME_LIMIT_MS 200
#endif

// Enter AT command mode.  We do this by doing the sleep-send_+++-sleep
// ritual, thouroughly flushing the receive buffer, and then sending a blank
// command and expecting an "OK\r" response.  Note that if some fiend is
// sending an endless string of "OK\r" strings on the network_id/channel
// the XBee is configured to use, we might be fooled into thinking we've
// made it to command mode when we haven't.  This routine returns TRUE if it
// thinks AT command mode has been entered successfully, or FALSE otherwise.
uint8_t
wx_enter_at_command_mode (void);

// Leave command mode (by sending the AT CN command).
uint8_t
wx_exit_at_command_mode (void);

// Require the XBee module to be in AT command mode (see
// wx_enter_at_command_mode()).  Check if the XBee network ID (ID parameter)
// is set to id, and if not, set it to id and save the settings.  The new
// setting is saved to saved to non-volatile memory when this command is
// issued, but doesn't actually take effect until AT command mode is exited
// (or an 'AC' command is issued).  Valid id values are 0x00 - 0xffff.
// NOTE: this command may permanently alter the XBee configuration (it can
// be restored using wx_restore_defaults().
uint8_t
wx_ensure_network_id_set_to (uint16_t id);

// Require the XBee module to be in AT command mode (see
// wx_enter_at_command_mode()).  Check if the XBee channel (CH parameter)
// is set to channel, and if not, set it to channel and save the settings.
// The new setting is saved to saved to non-volatile memory when the command
// is issued, but doesn't actually take effect until AT command mode is
// exited (or an 'AC' command is issued).  Valid channel values are 0x0b
// - 0x1a.  NOTE: this command may permanently alter the XBee configuration
// (it can be restored using wx_restore_defaults()).
uint8_t
wx_ensure_channel_set_to (uint8_t channel);

// Require the XBee module to be in AT command mode (see
// wx_enter_at_command_mode()).  Restore the XBee factory default
// configuration, and save the settings.
uint8_t
wx_restore_defaults (void);

// I don't think the Sparkfun WRL-10854 gives us any connection to the
// SLEEP_RQ pin of the XBee module, so FIXXME: this is unimplemented.
// However, hibernating is probably the first thing you'll want to do for
// a battery operated device, so its too bad we can't easily prototype it
// using the Arduino.  A few hints:
//
//   * Setting the SM parameter to 1 (using wx_com_expect_ok() once to set
//     the parameter and again to save the parameters) and then using
//     the WX_SLEEP() macro (which requires WX_SLEEP_RQ_CONTROL_PIN to
//     be defined first) from this interfaces gives a very simple way to
//     cut Xbee module power consumption to about 10 uA, with the only
//     disadvantage being that the sleepy node will have to wake itself up
//     (it cannot be called awake from a coordinator).
//
//   * An all-software solution which reduces power consumption to about 50 uA
//     is also possible, but it requires significantly more module
//     confiruation in order to establish a coordinator node, end devices
//     nodes, etc.
//
//   * The XBee Product manual version v1.xEx (a copy is in this module's
//     directory) has a description of the sleep mode options on page 23.
//
//void
//wx_hibernate (void);

// To actually send/receive bytes over the air with the default XBee
// configuration, you can just send them to the serial port.  See the
// corresponding UART_*() in uart.h for details.  If you want your
// transmissions to arrive atomically (not interleaved with other
// transmissions) see the wx_put_*_frame() routines.
#define WX_PUT_BYTE(byte)               UART_PUT_BYTE (byte)
#define WX_BYTE_AVAILABLE()             UART_BYTE_AVAILABLE()
#define WX_WAIT_FOR_BYTE()              UART_WAIT_FOR_BYTE()
#define WX_UART_RX_ERROR()              UART_RX_ERROR()
#define WX_UART_RX_FRAME_ERROR()        UART_RX_FRAME_ERROR()
#define WX_UART_RX_DATA_OVERRUN_ERROR() UART_RX_DATA_OVERRUN_ERROR()
#define WX_GET_BYTE()                   UART_GET_BYTE()
#define WX_UART_FLUSH_RX_BUFFER()       UART_FLUSH_RX_BUFFER()

///////////////////////////////////////////////////////////////////////////////
//
// Simple Frame Interface (NOT using XBee API mode)
//
// This section contains constants and routines that let you bundle data into
// short frames which:
//
//   * are guaranteed to arrive at recievers not interleaved with other data
//   * include CRC values
//   * can be conveniently received and verified (using wx_get_frame())
//
// Transmission can fail for a variety of reasons and acknowledgement
// messages, retries, etc. are the responsibility of clients of this
// interface.

// This interface assumes the XBee is being used in transparent mode, with
// the packetization timeout configuration parameter (R0) set to its default
// value.  Under these circumstances, small amounts of data sent quickly
// and continuously to the XBee will be lumped into single radio packets.
// Complete packets shorter than WX_TRANSPARENT_MODE_MAX_PACKET_SIZE
// bytes can be transmitted simply by not sending any bytes for at least
// WX_TRANSPARENT_MODE_PACKETIZATION_TIMEOUT_BYTES worth of time.
#define WX_TRANSPARENT_MODE_MAX_PACKET_SIZE 100
#define WX_TRANSPARENT_MODE_PACKETIZATION_TIMEOUT_BYTES 0x03

// The CRC and payload portions of frames will have the following byte
// values prefixed by an escape byte when they occur: 0x7E (ASCII '~'),
// 0x7D (ASCII '}'), 0x11 (ASCII device control 1, and 0x13 (ASCII device
// control 3).  If the data supplied to the frame transmission function
// contains many values that need to be escaped, the escaped frame size can
// end up exceeding WX_TRANSPARENT_MODE_MAX_PACKET_SIZE bytes.  However, the
// size of escaped data is at most twice its unescaped size.  We therefore
// have maximum safe sizes for unescaped payloads, and for unescaped payloads
// that include no bytes that need to be escaped.
#define WX_FRAME_DELIMITER_LENGTH 1   // Leading frame delimiter isn't escaped
#define WX_FRAME_LENGTH_FIELD_LENGTH 2
#define WX_FRAME_MAX_CRC_BYTES_WITH_ESCAPES 8
#define WX_FRAME_MAX_PAYLOAD_ESCAPE_EXPANSION_FACTOR 2
#define WX_FRAME_SAFE_PAYLOAD_LENGTH_WITH_NO_BYTES_REQUIRING_ESCAPE \
  (   WX_TRANSPARENT_MODE_MAX_PACKET_SIZE \
    - WX_FRAME_DELIMITER_LENGTH \
    - WX_FRAME_LENGTH_FIELD_LENGTH \
    - WX_FRAME_MAX_CRC_BYTES_WITH_ESCAPES )
#define WX_FRAME_SAFE_UNESCAPED_PAYLOAD_LENGTH \
  ( WX_FRAME_SAFE_PAYLOAD_LENGTH_WITH_NO_BYTES_REQUIRING_ESCAPE / \
    WX_FRAME_MAX_PAYLOAD_ESCAPE_EXPANSION_FACTOR )

// See below for details on these (you may not need to know)
#define WX_LENGTH_BYTE_XORED     0xff
#define WX_LENGTH_BYTE_NOT_XORED 0x00

// Put count bytes of data out from buf out over the air as a single radio
// packet containing a simple frame format.  This frame format features a
// delimiter, length metadata, and CRC protection.
//
// Besides taking some care that data segments don't get too long due to
// escaping (see comments above WX_FRAME_SAFE_UNESCAPED_PAYLOAD_LENGTH),
// you shouldn't need to know the gruesome details of this frame format if
// you'll be reading it with wx_get_frame().  But in case you aren't, here
// are the details:
//
// Certain byte values will need to be escaped when they occur (except
// the delimiter when it appears as the delimiter), which usually involves
// expanding them into two byte sequences (but see below).  The entire escaped
// byte sequence must not be longer than WX_TRANSPARENT_MODE_MAX_PACKET_SIZE
// bytes long.  This frame format and escaping scheme is like the one used by
// the XBee in API mode (see the API Operation section of the XBee Product
// Manual), with the following differences:
//
//   * The length field is two bytes long, but the first byte is just a flag
//     indicating whether the second byte should be xor'ed in XBee API mode.
//     The flag byte has value WX_LENGTH_BYTE_XORED if the next byte has
//     been xor'ed, or WX_LENGTH_BYTE_NOT_XORED otherwise.  The purpose of
//     this arrangement is to help avoid undetected errors that can result
//     from corruption in the payload length field.
//
//   * Immediately following the length field is a two-byte CRC computed from
//     the frame delimiter and the length bytes (the escape flag and the
//     possibly xor'ed length-indicating byte itself).  Corrupted lengh
//     bytes are by far the weakest point in most implementations that
//     use checksums or CRCs, including probably the one available
//     in XBee API mode; see http://www.ece.cmu.edu/~koopman/pubs/
//     01oct2013_koopman_faa_final_presentation.pdf.  Note that the bytes
//     of this CRC might themselves need to be escaped, if so this is done
//     as described in the XBee API mode documentation (resulting in a
//     sequence of up to four bytes).
//
//   * The payload checksum is two bytes long, and is computed from the
//     escaped payload contents using the 16 bit CRC-CCITT calculation
//     described in the util/crc16.h header of AVR libc.  Note that the
//     individual bytes of this CRC might themselves need to be escaped,
//     resulting in a sequence of up to four bytes.
//
// The data is first scanned to determine its length after escape bytes
// are added.  If the escaped data sequence is too long to go in a single
// radio packet, nothing is transmitted and false is returned (unless
// WX_ASSERT_SUCCESS is defined, in which case an assertion violation
// is triggered).  Otherwise the packet is transmitted and true is returned.
//
// These frames are not in any way compatible with the XBee API mode frames.
//
uint8_t
wx_put_frame (uint8_t count, void const *buf);

// Convenience wrappar around wx_put_frame().  If NUL-terminated string str
// is longer than UINT8_MAX - 1 (which is too long to go in one of our frames
// anyway) an assertion violation will be triggered.  The str argument must
// be NUL-terminated, but the trailing NUL is not transmitted as part of
// the data frame (the frame knows how long it is by other means anyway).
// See the description of the underlying wx_put_frame() for more details.
uint8_t
wx_put_string_frame (char const *str);


// FIXME: put somewhere better or remove or something
#ifndef __GNUC__
#  error __GNUC__ not defined
#endif

// Convenience wrapper around wx_put_string_frame().  The expanded string
// must not be longer than WX_FRAME_SAFE_UNESCAPED_PAYLOAD_LENGTH bytes.
// Returns TRUE on success, or FALSE on error.
// FIXME: printf_P version?
uint8_t
wx_put_string_frame_printf (char const *format, ...)
  __attribute__ ((format (printf, 1, 2)));

// Spend about timeout milliseconds trying to receive a frame with up to mfps
// (Maximum Frame Payload Size) unescaped payload bytes into buf.  The size of
// the payload received is returned in *rfps (Received Frame Payload Size).
// Regardless of the definedness of WX_ASSERT_SUCCESS, this routine returns
// TRUE if a full frame is successfully received, and false otherwise.
// Any partial or corrupt frame data received from the XBee is effectively
// discarded, though some of it might end up getting written into *buf.
// This function grabs a slice of incoming data starting when called and
// ending when either a valid frame is received, or a frame that has been
// started (due to the appearance of a frame delimiter in the data stream)
// turns out to be invalid or times out.  Therefore:
//
//   * Callers must be prepared to retry.  A frame could cross the
//     timeout boundry, or be corrupted.
//
//   * Transmitters must be prepared to resend their message (presumably
//     until they get some sort of acknowledgement).
//
//   * Leading non-frame (or partial fram) data may be discarded even if a
//     frame is eventually received successfully.
//
//   * Leading non-frame data that contains a frame delimiter byte (0x7E) will
//     inevitably result in what looks like a malformed frame, causing this
//     routine to attempt to read a frame and fail.
//
//   * If this function fails, it probably a good idea to call
//     WX_UART_FLUSH_RX_BUFFER() before attempting to receive any additional
//     data.  A data overrun can easily occur after such a failure,
//     which will leave WX_UART_RX_ERROR() true, which might confuse
//     other functions that check for errors when a byte is available.
//     Well-written functions should flush the buffer themselves when they
//     encounter a UART receiver error, but the results can still be confusing
//     since that other function will be seeing an error that's left over
//     from the aftermath of a call to this function.  Note that the actual
//     return from this function doesn't take much time on success or failure
//     (its fast enough that successive calls can pick up successive frames
//     sent in the same radio packet).  Its just that when failure occurs,
//     other things tend to be need doing that cause enough delay that a
//     serial overrun occurs.  The same thing can happen with success if
//     there's extra radio data floating around and your polling loop isn't
//     tight enough.  In other words, this is just a particularly likely
//     instance of the general class of problems that can occurn when you
//     don't poll fast enough and fail to flush the receiver buffer and
//     clear error flags after a failure.
//
//   * Its reasonable to first use WX_BYTE_AVAILABLE() from a polling loop to
//     determine when it might be worthwhile to call this routine.
//
// Using short timeout values is asking for trouble.  Although the serial
// connection to the XBee goes at about one byte per millisecond, and
// the XBee to XBee RF link is theoretically even faster, its probably
// a bad idea to depend on these rates.  Who knows what the XBee does?
// It may be laggy at the start of transmissions, or have RF packetization
// overhead, or take longer when there's noise.  Using WX_BYTE_AVAILABLE()
// before trying this function improves the success rate for a given timeout
// setting, because it ensures that none of the timeout period is wasted
// before the frame even starts.
//
uint8_t
wx_get_frame (uint8_t mfps, uint8_t *rfps, void *buf, uint16_t timeout);

// Spend about timeout milliseconds trying to receive a frame containing
// a string of up to msl characters into str.  A trailing NUL byte is
// automatically added if the incoming string doesn't already end with one.
// The memory pointed to by str should be at least msl + 1 bytes long (for
// the possible trailing NUL).  This is a thin wrapper around wx_get_frame().
uint8_t
wx_get_string_frame (uint8_t msl, char *str, uint16_t timeout);

///////////////////////////////////////////////////////////////////////////////
//
// Low Level/Extension Interface
//
// The remaining functions in this header are only useful if you need to
// change the XBee module configuration significantly.
//
// Note that there are many changes you can make to the XBee configuration
// which will violate the assumptions made by other parts of the interface.

// Maximum Command Output String Length (in bytes).  This includes any
// trailing carriage return ('\r') or NUL bytes that may be involved,
// and so is a safe size of buffer to use with wx_at_command().
#define WX_MCOSL 15

// Require the XBee module to be in AT command mode (see
// wx_enter_at_command_mode()).  Execute the given AT command with an
// "AT" prefix and "\r" postfix implicitly added (e.g. "BD9600" becomes
// "ATBD9600"), place the command output in output, stip the trailing carriage
// return ("\r") from output, and finally return TRUE if all that succeeded.
// Both command and output should be pointers to at least WX_MCOSL bytes
// of storage.  Its ok to pass the same pointer for both command and output,
// in which case the command string is overwritten with the command output
// (saving a few bytes of RAM).  The command string should be NUL-terminated,
// and the output will be NUL-terminated.
uint8_t
wx_at_command (char *command, char *output);

// Require the XBee module to be in AT command mode (see
// wx_enter_at_command_mode()).  Calls wx_at_command(), and the given
// command is expected to ouput "OK".  Return true iff everything wx_com()
// would do works and we get an OK back.
uint8_t
wx_at_command_expect_ok (char const *command);

#endif // WIRELESS_XBEE_H
