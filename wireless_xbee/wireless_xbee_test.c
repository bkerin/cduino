#include <stdlib.h>

#include "uart.h"
  
int
main (void)
{
  uart_init ();

  // FIXME: this stuff should be moved over to the uart module

#define CHARS_TO_GET 5
  uint8_t received_chars[CHARS_TO_GET];
  uint8_t ii;

  for ( ii = 0 ; ii < CHARS_TO_GET ; ii++ ) {
    if ( UART_RX_ERROR () ) {
      exit (1);
    }
    else {
      received_chars[ii] = uart_get_byte ();
    }
  }

  uart_put_byte ('b');
  uart_put_byte ('o');
  uart_put_byte ('o');
  uart_put_byte ('g');
  uart_put_byte ('e');
  uart_put_byte ('r');
  uart_put_byte (':');
  uart_put_byte (' ');
  for ( ii = 0 ; ii < CHARS_TO_GET ; ii++ ) {
    uart_put_byte (received_chars[ii]);
  }
  uart_put_byte ('\r');
  uart_put_byte ('\n');
}
