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

  uint8_t sentinel = wx_com ("BD", co);

  // FIXME: explain that the magic "3" means 9600
  if ( sentinel && (! strcmp (co, "3\r")) ) {
    for ( ; ; ) { CHKP (); }   // Blinking means it worked!
  }
  else {
    for ( ; ; ) { ; }
  }
}
