#include <assert.h>
#include <stdlib.h>  // FIXME: probably only needed for broken assert.h 
#include <string.h>
#include <util/delay.h>

#include "uart.h"
#include "util.h"
#include "wireless_xbee.h"
  
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

  //wx_ensure_network_id_set_to (0x3332);
  //exit (0);

  for ( ; ; ) { CHKP (); }   // Blinking means everything worked :)
}
