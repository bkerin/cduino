// Implementation of the interface described in term_io.h.

#include <stdio.h>

#include "term_io.h"
#include "uart.h"

static FILE uart_str
  = FDEV_SETUP_STREAM (uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void
term_io_init (void)
{
  uart_init ();

  stdout = stdin = &uart_str;
}

int
term_io_getline (char *linebuf)
{
  char *lbp = linebuf;
    
  size_t char_count = 0;

  for ( ; ; ) {

    int c = uart_getchar (stdin);

    switch ( c ) {
      case -1:
        return -1;
        break;
      // FIXME: what do we do with _FDEV_EOF (and when does it happen).
      case _FDEV_EOF:
      case '\n':
        // FIXME: we should do something intelligent when we're about to
        // overflow the buffer.  For now we just return -1, maybe we should
        // stuff an error message in the returned string?
        *lbp = c;
        lbp++;
        char_count++;
        if ( char_count == TERM_IO_LINE_BUFFER_MIN_SIZE ) {
          return -1;
        }
        *lbp = '\0';
        return char_count;
        break;
      default:
        *lbp = c;
        lbp++;
        char_count++;
        // FIXME: see other fixme
        if ( char_count == TERM_IO_LINE_BUFFER_MIN_SIZE ) {
          return -1;
        }
        break;
    }
  }  
}
