

#include <assert.h>
#include <stdlib.h>  // FIXME: probably only needed for broken assert.h 
#include <string.h>
#include <util/delay.h>

#include "uart.h"
#include "util.h"
#include "wireless_xbee.h"

// FIXME: probably put a pointer to this story somewhere else too?:
// The below macro is like the CHKP() macro from util.h, but it used pin
// Digital 4 (aka PD4 in ATmega328P-speak).  Why, you ask?  Well, here's
// the story:
//
//   The (FIXME: XBee shielf ref) uses the hardware serial port to talk to the
//   XBee module.  Therefore:
//
//   Your host computer cannot use serial-over-USB to program the Arduino
//   unless you flip the tiny switch on the (FIXME: shield ref) to the 'DLINE'
//   position.  After uploading you have to switch it back and push the tiny
//   reset button.  This gets old fast.  Therefore:
//
//   You might want to use an AVRISPmkII to upload this test driver (assuming
//   you'll be developing off it).  The build system supports this (see the
//   description near UPLOAD_METHOD in generic.mk for warnings and details).
//   However, you'll discover that the plug won't fit in the in-system
//   programming header with the (FIXME: shield ref) fully installed. DO NOT
//   try to use the (FIXME: shield ref) without it being fully plugged in,
//   I can testify from experience that this can result in flaky connections
//   and much frustration.  Instead, just use a set of stacking blocks
//   (ref: where to get them) to raise the (FIXME: shield ref) up high
//   enough that the ISP cable will fit.  Don't have extra stacking blocks?
//   Then read on...
//
//   I found that my Official Arduino Motor Shield R3 has long tails and
//   leaves the ISP header unconnected, so I just plugged that in under the
//   (FIXME: shield ref).  But the motor shield uses PB5 for its own purposes
//   and I didn't want to confuse it, hence this macro.  If you remember
//   to add a LED from PD4 to ground (with a current-limiting resistor if
//   you're feeling prim and proper) you'll have a nice working test setup
//   that doesn't require you to twiddle the tiny switch and button every
//   edit-compile-debug :)
//
#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 3)

int
main (void)
{
  wx_init ();

  char co[WX_MCOSL];   // Command Output
  uint8_t sentinel;    // For sentinel values returned by functions

  // The AT command which queries the current baud setting returns a string
  // containing a particular number followed by a carriage return ('\r')
  // to mean 9600 baud (and wx_com() will strip off the trailing '\r' for us).
#define STRING_MEANING_9600_BAUD "3"

  // Check that the current baud setting is 9600.  This is the default setting
  // for the XBee modules, and is the only setting this interface supports,
  // so that's what we should see if we see anything.
  sentinel = wx_com ("BD", co);
  assert (sentinel);
  assert (! strcmp (co, STRING_MEANING_9600_BAUD));

  // The default settings for the XBee module for a couple parameter we
  // can tweak.
#define DEFAULT_NETWORK_ID 0x3332
#define DEFAULT_CHANNEL 0x0c

  // Some non-default setting that we're going to try out
#define NON_DEFAULT_NETWORK_ID 0x3342
#define NON_DEFAULT_CHANNEL 0x14

  // Equivalent values in the string forms used by the AT command set
#define DEFAULT_NETWORK_ID_STRING "3332"
  // WARNING: it appears that the XBee doesn't print the leading zeros
  // when responding to queries (it still accepts leading zeros when
  // values are being set, and for all I know they may be required in
  // that context).  Therefore we have both DEFAULT_CHANNEL_STRING and
  // OTHER_POSSIBLE_DEFAULT_CHANNEL_STRING for the channel case.  Presumably
  // this is an example of a general behavior that's worth being aware of
  // if you need to query configuration settings.
#define DEFAULT_CHANNEL_STRING "0C"
  // See comment above near the DEFAULT_CHANNEL_STRING define.
#define OTHER_POSSIBLE_DEFAULT_CHANNEL_STRING "C"
#define NON_DEFAULT_NETWORK_ID_STRING "3342"
#define NON_DEFAULT_CHANNEL_STRING "14"
  
  // Test wx_ensure_network_id_set_to()
  sentinel = wx_ensure_network_id_set_to (NON_DEFAULT_NETWORK_ID);
  assert (sentinel);
  sentinel = wx_com ("ID", co);
  assert (sentinel);
  assert (! strcmp (co, NON_DEFAULT_NETWORK_ID_STRING));

  // Test wx_ensure_channel_set_to()
  sentinel = wx_ensure_channel_set_to (NON_DEFAULT_CHANNEL);
  assert (sentinel);
  sentinel = wx_com ("CH", co);
  assert (sentinel);
  assert (! strcmp (co, NON_DEFAULT_CHANNEL_STRING));

  // Test wx_restore_defaults()
  sentinel = wx_restore_defaults ();
  sentinel = wx_com ("ID", co);
  assert (sentinel);
  assert (! strcmp (co, DEFAULT_NETWORK_ID_STRING));
  sentinel = wx_com ("CH", co);
  assert (sentinel);
  assert (
      (! strcmp (co, DEFAULT_CHANNEL_STRING)) ||
      (! strcmp (co, OTHER_POSSIBLE_DEFAULT_CHANNEL_STRING)) );

  // This first batch of checkpoint blinks mean all the setup stuff worked :)
  CHKP_PD4 ();

  // FIXME: test over-the-air here, and explain somewhere how to set up
  // transmit from USB thingy to test other half of it.  Also explain blinky
  // test conditions at top of this file probably.
}
