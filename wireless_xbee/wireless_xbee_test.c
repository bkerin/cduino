#include <assert.h>
#include <stdlib.h>  // FIXME: probably only needed for broken assert.h 
#include <util/delay.h>

#include "uart.h"
#include "util.h"
  
int
main (void)
{
  uart_init ();

  // Sequence to go into command mode
  _delay_ms (1200);
  UART_PUT_BYTE ('+');
  UART_PUT_BYTE ('+');
  UART_PUT_BYTE ('+');
  _delay_ms (1200);
    
  // Get a byte
  UART_WAIT_FOR_BYTE ();
  assert (! UART_RX_ERROR ());
  assert (sizeof (char) == 1);
  char b1 = UART_GET_BYTE ();

  // Get another byte
  UART_WAIT_FOR_BYTE ();
  assert (! UART_RX_ERROR ());
  assert (sizeof (char) == 1);
  char b2 = UART_GET_BYTE ();

  // See if we got OK indicating command mode
  if ( b1 == 'O' && b2 == 'K' ) {
    for ( ; ; ) { CHKP (); }
  }

}
