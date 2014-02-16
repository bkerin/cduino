// Test/demo for the wireless_xbee.h interface.
//
// Unlike many other Cduino test programs, this one doesn't blink the on-board
// PB5 LED, but instead requires you to connect a led from Digital 4 (PD4)
// to ground.  For much of the testing an second XBee tranciever and external
// software driver are also required, see the details below.

#include <assert.h>
#include <stdlib.h>  // FIXME: probably only needed for broken assert.h 
#include <string.h>
#include <util/delay.h>

#include "util.h"
#include "wireless_xbee.h"

// The below macro is like the CHKP() macro from util.h, but it uses pin
// Digital 4 (aka PD4 in ATmega328P-speak).  Why, you ask?  Well, here's
// the story:
//
//   The Sparkfun XBee Shield (Sparkfun part number WRL-10854) uses the
//   hardware serial port to talk to the XBee module (so do all other XBee
//   shield sthat I'm aware of).  Therefore:
//
//   Your host computer cannot use serial-over-USB to program the Arduino
//   unless you flip the tiny switch on the shield to the 'DLINE' position.
//
//   After uploading you have to switch it back and push the tiny reset
//   button.  This gets old fast.  Therefore:
//
//   You might want to use an AVRISPmkII to upload this test driver (assuming
//   you'll be developing off it).  The build system supports this (see the
//   description near UPLOAD_METHOD in generic.mk for warnings and details).
//   However, you'll discover that the plug won't fit in the in-system
//   programming header with the Sparkfun WRL-10854 fully installed (the
//   official Arduino XBee shield might be better in this regard, as they
//   have been leading the charge towards using longer tails on the stacking
//   blocks). DO NOT try to use the shield without it being fully plugged
//   in, I can testify that this can result in flaky connections and much
//   frustration.  Instead, just use a set of stacking blocks to raise the
//   shield up high enough that the ISP cable will fit.  Don't have extra
//   stacking blocks?  Then read on...
//
//   I found that my Official Arduino Motor Shield R3 has long tails and
//   leaves the ISP header unconnected, so I just plugged that in under the
//   Sparkfun WRL-10854.  But the motor shield uses PB5 for its own purposes
//   and I didn't want to confuse it, hence this macro.  If you remember
//   to add a LED from PD4 to ground (with a current-limiting resistor if
//   you're feeling prim and proper) you'll have a nice working test setup
//   that doesn't require you to twiddle the tiny switch and button every
//   edit-compile-debug :)
//
#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 3)

// Single-blink checkpoint (or other thing) version
#define CHKP_PD4_SB() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 1)

// FIXME: Put something like this in util.
#define HYPB() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 100.0, 50)
#define FIXME_SERT(condition) \
  do { if (! (condition)) { for ( ; ; ) { HYPB(); } } } while ( 0 );

int
main (void)
{
  wx_init ();

  uint8_t sentinel;    // For sentinel value returned by many functions

// Uncomment this to enable to arduino to act as a responder that can be
// substituted for the ./usb_xbee_test --query-mode instance as described in
// the comments at the start of usb_xbee_test.
//#define AUTOMATIC_TESTING_WITH_USB_XBEE_TEST

#ifndef AUTOMATIC_TESTING_WITH_USB_XBEE_TEST
  
  char co[WX_MCOSL];   // Command Output
  
  // The AT command which queries the current baud setting returns a string
  // containing a particular number followed by a carriage return ('\r')
  // to mean 9600 baud (and wx_com() will strip off the trailing '\r' for us).
#  define STRING_MEANING_9600_BAUD "3"

  sentinel = wx_enter_at_command_mode ();
  assert (sentinel);

  // Check that the current baud setting is 9600.  This is the default setting
  // for the XBee modules, and is the only setting this interface supports,
  // so that's what we should see if we see anything.
  sentinel = wx_at_command ("BD", co);
  assert (sentinel);
  assert (! strcmp (co, STRING_MEANING_9600_BAUD));

  // The default settings for the XBee module for a couple parameter we
  // can tweak.
#  define DEFAULT_NETWORK_ID 0x3332
#  define DEFAULT_CHANNEL 0x0c

  // Some non-default setting that we're going to try out
#  define NON_DEFAULT_NETWORK_ID 0x3342
#  define NON_DEFAULT_CHANNEL 0x14

  // Equivalent values in the string forms used by the AT command set
#  define DEFAULT_NETWORK_ID_STRING "3332"
  // WARNING: it appears that the XBee doesn't print the leading zeros
  // when responding to queries (it still accepts leading zeros when
  // values are being set, and for all I know they may be required in
  // that context).  Therefore we have both DEFAULT_CHANNEL_STRING and
  // OTHER_POSSIBLE_DEFAULT_CHANNEL_STRING for the channel case.  Presumably
  // this is an example of a general behavior that's worth being aware of
  // if you need to query configuration settings.
#  define DEFAULT_CHANNEL_STRING "0C"
  // See comment above near the DEFAULT_CHANNEL_STRING define.
#  define OTHER_POSSIBLE_DEFAULT_CHANNEL_STRING "C"
#  define NON_DEFAULT_NETWORK_ID_STRING "3342"
#  define NON_DEFAULT_CHANNEL_STRING "14"

  // Test wx_ensure_network_id_set_to()
  sentinel = wx_ensure_network_id_set_to (NON_DEFAULT_NETWORK_ID);
  assert (sentinel);
  sentinel = wx_at_command ("ID", co);
  assert (sentinel);
  assert (! strcmp (co, NON_DEFAULT_NETWORK_ID_STRING));

  // Test wx_ensure_channel_set_to()
  sentinel = wx_ensure_channel_set_to (NON_DEFAULT_CHANNEL);
  assert (sentinel);
  sentinel = wx_at_command ("CH", co);
  assert (sentinel);
  assert (! strcmp (co, NON_DEFAULT_CHANNEL_STRING));

  // Test wx_restore_defaults()
  sentinel = wx_restore_defaults ();
  sentinel = wx_at_command ("ID", co);
  assert (sentinel);
  assert (! strcmp (co, DEFAULT_NETWORK_ID_STRING));
  sentinel = wx_at_command ("CH", co);
  assert (sentinel);
  assert (
      (! strcmp (co, DEFAULT_CHANNEL_STRING)) ||
      (! strcmp (co, OTHER_POSSIBLE_DEFAULT_CHANNEL_STRING)) );
  
  sentinel = wx_exit_at_command_mode ();
  assert (sentinel);

  // This first batch of blinks mean all the AT command stuff worked :)
  CHKP_PD4 ();

  float tbt_ms = 1000;   // Time Between Tests (in milliseconds)
  _delay_ms (tbt_ms);

  // The remainder of this test program depends on having a Sparkfun XBee
  // Explorer USB (Sparkfun part number WRL-08687) or equivalent (probably any
  // USB adapter based on the FTDI FT232RL will work) and a copy of usb_xbee
  // running in framed lines mode.  If you're Explorer is associated with
  // device file /dev/ttyUSB0 (see the POD documentation in usb_xbee for a
  // way to check) invocation would look like this:
  //
  //   ./usb_xbee -d /dev/ttyUSB0 -f
  //
  // Now we repeatedly send out prompts for the answer, blinking a light
  // whenever we get it (the answer being 42 of course :).  Regardless of the
  // answer given, we echo each received string back out in a return frame.
  //
  // Note that if this test program is started on the Arduino before usb_xbee
  // is started, usb_xbee may fail at startup complaining about a timeout
  // before receiving a full frame.  This is expected and correct behavior.
  // Just try starting it again.  It might also fail later if there is
  // non-frame radio data floating around on the network/channel in use.
  // Its supposed to do that.

  while ( 1 ) {

    uint8_t sentinel = wx_put_string_frame ("What is the answer?\n"); 
    assert (sentinel);

    uint16_t tpra_ms = 2042;   // Time Per Read Attempt (in milliseconds)

    // Ok, some of the interface macros are long and ugly.  Here's a short
    // ugly alias for our private use: Max Payload Length For Us.
#  define MPLFU WX_FRAME_SAFE_PAYLOAD_LENGTH_WITH_NO_BYTES_REQUIRING_ESCAPE   
    // Testing of frames of maximum length, with many escape characters, etc.
    // is handled largely from usb_xbee_test, but in case you want to try to
    // verify that such frames always work on the C side you can use these
    // line instead of the above.
//#  define MPLFU WX_FRAME_SAFE_UNESCAPED_PAYLOAD_LENGTH 
  
    char rstr[MPLFU + 1];   // Received string

    sentinel = wx_get_string_frame (MPLFU + 1, rstr, tpra_ms);
    if ( ! sentinel ) {
      // In kindness to other callers, we clean up after any UART Rx error.
      if ( WX_UART_RX_ERROR () ) {
        WX_UART_FLUSH_RX_BUFFER ();
      }
      // Timeouts, bad frames, all sorts of errors end up getting eaten
      // here FIXXME: the frame functions should probably do some sort of
      // error propagation.
      continue;
    }

#  define THE_ANSWER "42\n"

    if ( ! strcmp (rstr, THE_ANSWER) ) {
      CHKP_PD4 ();
    }

    sentinel = wx_put_string_frame (rstr);
    assert (sentinel);
  }

#else // AUTOMATIC_TESTING_WITH_USB_XBEE_TEST is defined

  while ( 1 ) {

    uint16_t tpra_ms = 2042;   // Time Per Read Attempt (in milliseconds)

    // Ok, some of the interface macros are long and ugly.  Here's a short
    // ugly alias for our private use: Max Payload Length For Us.
#define MPLFU WX_FRAME_SAFE_PAYLOAD_LENGTH_WITH_NO_BYTES_REQUIRING_ESCAPE   
    // Testing of frames of maximum length, with many escape characters, etc.
    // is handled largely from usb_xbee_test, but in case you want to try to
    // verify that such frames always work on the C side you can use these
    // line instead of the above.
//#define MPLFU WX_FRAME_SAFE_UNESCAPED_PAYLOAD_LENGTH 
  
    uint8_t rfps;
    char rpyld[MPLFU];   // Received payload
 
    sentinel = wx_get_frame (MPLFU, &rfps, rpyld, tpra_ms);
    if ( ! sentinel ) {
      // In kindness to other callers, we clean up after any UART Rx error.
      if ( WX_UART_RX_ERROR () ) {
        WX_UART_FLUSH_RX_BUFFER ();
      }
      // Timeouts, bad frames, all sorts of errors end up getting eaten
      // here FIXXME: the frame functions should probably do some sort of
      // error propagation.
      continue;
    }
    CHKP_PD4_SB ();

    sentinel = wx_put_frame (rfps, rpyld);
    assert (sentinel);

  }

#endif

}
