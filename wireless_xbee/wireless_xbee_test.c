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

//#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 3)
#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 1)

// Put something like this in util.
// FIXME: extra debug
#define HYPB() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 100.0, 50)
#define FIXME_SERT(condition) do { if (! (condition)) { for (;;) {HYPB();} } } while ( 0 );

int
main (void)
{
  wx_init ();

  uint8_t sentinel;    // For sentinel value returned by many functions

  while ( 1 ) {

    uint16_t tpra_ms = 6042;   // Time Per Read Attempt (in milliseconds)

#define MPLFU WX_FRAME_SAFE_PAYLOAD_LENGTH_WITH_NO_BYTES_REQUIRING_ESCAPE   
  
    char rstr[MPLFU + 1];   // Received string

    sentinel = wx_get_string_frame (MPLFU + 1, rstr, tpra_ms);
    if ( ! sentinel ) {
      // In kindness to other callers, we do this to get rid of any leftover
      // data and clear the UART error flags after a failure.
      // FIXME: do we want this?
      if ( WX_UART_RX_ERROR () ) {
        WX_UART_FLUSH_RX_BUFFER ();
      }
      continue;
    }
    CHKP_PD4 ();

    //_delay_ms (500);

    //sentinel = wx_put_string_frame (rstr);
    sentinel = wx_put_frame (44, rstr);
  }

}
