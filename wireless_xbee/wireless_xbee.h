// MaxStream XBee Series 1 (aka XBee 802.15.4) Wireless Module Interface
//
// Test driver: wireless_xbee_test.c    Implementation: wireless_xbee.c
//
// You really want to read this entire interface file, and maybe the
// referenced material as well.
//
// This module uses the ATmega328P hardware serial port to communicate
// with the XBee.  It features high-level support for a few configuration
// parameters that people are most likely to desire to change, some low-level
// functions for people who need to do more extensive XBee reconfiguration,
// some data Tx and Rx macros which just use the underlying serial interface,
// and a higher-level Tx/Rx interface featuring atomic data frames.
//
// Though this module should not be dependent on any particular shield,
// the Sparkfun XBee Shield (Sparkfun part number WRL-10854) was used
// for development.  Its available on its own, or as part of the Sparkfun
// "XBee Wireless Kit Retail" (Sparkfun part number RTL-11445), which also
// includes the actual XBee modules and a stand-alone miniature USB XBee
// interface board known as an "XBee Explorer USB" (Sparkfun part number
// WRL-08687) as well.  This last is a must-have for development IMO.
// Make sure to get a USB Type A to USB Mini-B cable as well, it isn't
// included in the kit.  Alternatively (if you aren't getting the whole kit)
// you can grab an XBee Explorer Dongle (Sparkfun part number WRL-09819),
// then you don't need the cable.
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
// the DEFAULT_CHANNEL_STRING define in wireless_xbee.h for an important
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

#ifndef WIRELESS_XBEE_H
#define WIRELESS_XBEE_H

#include <inttypes.h>

#include "uart.h"


///////////////////////////////////////////////////////////////////////////////
//
// About Errror Handling
//
// This module really doesn't do much of it.  It just returns true on succes,
// and false otherwise.  If the macro WX_ASSERT_SUCCES is defined it doesn't
// even do that: it simply calls assert() internally if something fails.
// In this case, all the function descriptions which indicate sentinel
// return values are wrong unless otherwise noted.
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

// Initialize the interface to the XBee.  Currently this interface only
// supports talking to XBee devices over the hardware serial port at WX_BAUD
// Baud, with eight data bits, no parity, and one stop bit (8-N-1 format).
// So the serial port is initialized with those parameters, and that's all
// this routine does.
void
wx_init (void);

// Uncomment this or otherwise arrange for this macro to be defined to
// enable simplified error handling.  See "About Error Handling" above.
//#define WX_ASSERT_SUCCESS

// Enter AT command mode by doing the sleep-send_+++-sleep ritual.
// This function takes a few seconds to execute, because entering command
// mode requires two guard times of 1 second or more.  The XBee module will
// automatically drop out of command mode after 10 seconds (unless the AT
// CT command has been used to reconfigure the module with a non-default
// timeout).  Returns true on success, false otherwise.
uint8_t
wx_enter_at_command_mode (void);

// Leave command mode (by sending the AT CN command).
uint8_t
wx_exit_at_command_mode (void);

// Require the XBee module to be in AT command mode (see
// wx_enter_at_command_mode()).  Check if the XBee network ID (ID parameter)
// is set to id, and if not, set it to id and save the settings.  Valid id
// values are 0x00 - 0xffff.  NOTE: this command may permanently alter the
// XBee configuration (it can be restored using wx_restore_defaults().
uint8_t
wx_ensure_network_id_set_to (uint16_t id);

// Require the XBee module to be in AT command mode (see
// wx_enter_at_command_mode()).  Check if the XBee channel (CH parameter) is
// set to channel, and if not, set it to channel and save the settings.  Valid
// channel values are 0x0b - 0x1a.  NOTE: this command may permanently alter
// the XBee configuration (it can be restored using wx_restore_defaults()).
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
//     the parameter and again to save the parameters) and then asserting
//     the SLEEP_RQ should give a very simple way to cut Xbee module power
//     consumption to about 10 uA, with the only disadvantage being that
//     the sleepy node will have to wake itself up (it cannot be called
//     awake from a coordinator).  Perhaps just soldering a lead to the
//     top of the SLEEP_RQ on the XBee module and plugging it into a DIO
//     line would be a good way to test this out.
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
// See the underlying wx_put_frame() description for more details.
uint8_t
wx_put_string_frame (char const *str);

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
// turns out to be invalid.  Therefore:
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
