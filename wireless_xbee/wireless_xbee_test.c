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
//   (FIXME: shield ref).  But it uses PB5 for its own purposes and I didn't
//   want to confuse it, hence this macro.  If you remember to add a LED from
//   PD4 to ground (with a current-limiting resistor if you're feeling prim
//   and proper) you'll have a nice working test setup that doesn't require
//   you to twiddle the tiny switch and button every edit-compile-debug :)
//
#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 3)

int
main (void)
{
  wx_init ();

  char co[WX_MCOSL];   // Command Output

  // The AT command which queries the current baud setting returns a string
  // containing a particular number followed by a carriage return ('\r')
  // to mean 9600 baud (and wx_com() will strip off the trailing '\r' for us).
#define STRING_MEANING_9600_BAUD "3"

  // Check that the current baud setting is 9600.  This is the default setting
  // for the XBee modules, and is the only setting this interface supports,
  // so that's what we should see if we see anything.
  uint8_t sentinel = wx_com ("BD", co);
  assert (sentinel);
  assert (! strcmp (co, STRING_MEANING_9600_BAUD));

  wx_ensure_network_id_set_to (0x3332);
  exit (0);

  //for ( ; ; ) { CHKP (); }   // Blinking means everything worked :)
}
