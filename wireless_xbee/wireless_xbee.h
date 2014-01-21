// MaxStream XBee Series 1 (aka XBee 802.15.4) Wireless Module Interface
//
// Test driver: wireless_xbee_test.c    Implementation: wireless_xbee.c
//
// This module uses the ATmega328P hardware serial port to communicate
// with the XBee.  It features high-level support for a few configuration
// parameters that people are most likely to desire to change, some low-level
// functions for people who need to do more extensive XBee reconfiguration,
// and some data Tx and Rx macros which just use the underlying serial
// interface.
//
// Though this module should not be dependent on any particular shield,
// the Sparkfun XBee Shield (Sparkfun part number WRL-10854) was used
// for development.  Its available on its own, or as part of the Sparkfun
// "XBee Wireless Kit Retail" (Sparkfun part number RTL-11445), which also
// includes the actual XBee modules and a stand-alone miniature USB XBee
// board that's extremely handy to have (FIXME: ref our perl script that
// uses it if it does).
//
// Sparkfun has IMO the best information page for XBee modules:
//
//   https://www.sparkfun.com/pages/xbee_guide
//
// There are a couple pages on the Arduino site that are worth reading,
// particularly if you need to do more extensive XBee configuration than
// what this interface provides directly.  WARNING: read the commend near
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
// Otherwise, make sure to take note of the tiny switch on the WRL-10854:
// it needs to be in the DLINE position for serial programming to work,
// and the UART position for communication between the Arduino and the
// XBee to work.  So you'll end up toggling the switch twice and pushing
// the reset button once per edit-compile-debug cycle.

#ifndef WIRELESS_XBEE_H
#define WIRELESS_XBEE_H

#include "uart.h"

// About Errror Handling
//
// This module really doesn't do much of it.  It just returns true on succes,
// and false otherwise.  If the macro WX_ASSERT_SUCCES is defined it doesn't
// even do that: it simply calls assert() internally if something fails.
// In this case, all the function descriptions which indicate sentinel
// return values are wrong :).
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
// Note that the actual over-the-air transmission (using WX_PUT_BYTE()) does
// not by itself result in any feedback at all about whether the transmission
// was actually received anywhere.  In the default point-to-multipoint
// XBee configuration, all nearby modules with the same network ID (see
// wx_ensure_network_id_set_to()) and channel (see wx_ensure_channel_set_to())
// will hopefully receive the byte, but its up to you to arrange for them
// to send back something saying they have if you really want to know.

// Initialize the interface to the XBee.  Currently this interface only
// supports talking to XBee devices over the hardware serial port at 9600
// Baud, with eight data bits, no parity, and one stop bit (8-N-1 format).
// So the serial port is initialized with those parameters, and that's it
// all this routine does.
void
wx_init (void);

// Uncomment this or otherwise arrange for this macro to be defined to
// enable simplified error handling.  See "About Error Handling" above.
//#define WX_ASSERT_SUCCESS

// Enter AT command mode by doing the sleep-send_+++-sleep ritual.  NOTE: the
// XBee module will automatically drop out of command mode after 10 seconds
// (unless the AT CT command has been used to reconfigure the module with
// a non-default timeout).  Returns true on success, false otherwise.  It 
uint8_t
wx_enter_at_command_mode (void);

// Leave command mode (by sending the AT CN command).
uint8_t
wx_exit_at_command_mode (void);

// Enter AT command mode and check if the XBee network ID (ID parameter)
// is set to id, and if not, set it to id, save the settings, and exit
// command mode.  Valid id values are 0x00 - 0xffff.  NOTE: this command
// may permanently alter the XBee configuration (it can be restored using
// wx_restore_defaults().
uint8_t
wx_ensure_network_id_set_to (uint16_t id);

// Enter AT command mode and check if the XBee channel (CH parameter)
// is set to channel, and if not, set it to channel, save the settings,
// and exit command mode.  Valid channel values are 0x0b - 0x1a.  NOTE:
// this command may permanently alter the XBee configuration (it can be
// restored using wx_restore_defaults()).
uint8_t
wx_ensure_channel_set_to (uint8_t channel);

// Enter AT command mode, restore the XBee factory default configuration,
// save the settings, and exit command mode.
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

// To actually send/receive data over the air, you just use the serial port.
// See the corresponding UART_*() in uart.h for details.  FIXME: file linked?
#define WX_PUT_BYTE(byte)               UART_PUT_BYTE (byte)
#define WX_BYTE_AVAILABLE()             UART_BYTE_AVAILABLE()
#define WX_WAIT_FOR_BYTE()              UART_WAIT_FOR_BYTE()
#define WX_UART_RX_ERROR()              UART_RX_ERROR()
#define WX_UART_RX_FRAME_ERROR()        UART_RX_FRAME_ERROR()
#define WX_UART_RX_DATA_OVERRUN_ERROR() UART_RX_DATA_OVERRUN_ERROR()
#define WX_GET_BYTE()                   UART_GET_BYTE()

///////////////////////////////////////////////////////////////////////////////
//
// Low Level/Extension Interface
//
// The remaining functions in this header are only useful if you need to
// change the XBee module configuration significantly.

// Maximum Command Output String Length (in bytes).  This includes any
// trailing carriage return ('\r') or NUL bytes that may be involved,
// and so is a safe size of buffer to use with wx_com().
#define WX_MCOSL 15

// Enter command mode, execute the given AT command with an "AT" prefix
// and "\r" postfix implicitly added (e.g. "BD9600" becomes "ATBD9600"),
// place the command output in output, stip the trailing carriage return
// ("\r") from output, and finally return TRUE if all that succeeded.
// Both command and output should be pointers to at least WX_MCOSL bytes
// of storage.  Its ok to pass the same pointer for both command and output,
// in which case the command string is overwritten with the command output
// (saving a few bytes of RAM).  The command string should be NUL-terminated,
// and the output will be NUL-terminated.
uint8_t
wx_com (char *command, char *output);

// Convenience function.  Like wx_com(), but the given command is expected
// to ouput "OK\r" (note that wx_com() would strip the trailing "\r").
// Return true iff everything wx_com() would do works and we get an OK back.
uint8_t
wx_com_expect_ok (char const *command);

#endif // WIRELESS_XBEE_H
