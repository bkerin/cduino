// Implementation of the interface described in term_io.h.

#include <stdio.h>

#include "term_io.h"
#include "uart.h"

static FILE term_io_str
  = FDEV_SETUP_STREAM (term_io_putchar, term_io_getchar, _FDEV_SETUP_RW);

void
term_io_init (void)
{
  uart_init ();

  stdout = stdin = &term_io_str;
}

int
term_io_putchar (char ch, FILE *stream)
{
  // FIXME: this fctn should dissapear from the interface

  if ( ch == '\n' ) {
    term_io_putchar ('\r', stream);
  }
  UART_PUT_BYTE (ch);

  return 0;
}

int
term_io_getchar (FILE *stream)
{
  // FIXME: this fctn should dissapear from the interface

  uint8_t ch;
  char *cp, *cp2;
  static char buf[RX_BUFSIZE];
  static char *rxp;

  if ( rxp == 0 ) {
    for ( cp = buf ; ; ) {
      UART_WAIT_FOR_BYTE ();
      if ( UART_RX_FRAME_ERROR () ) {
        return _FDEV_EOF;
      }
      if ( UART_RX_DATA_OVERRUN_ERROR () ) {
        return _FDEV_ERR;
      }
      ch = UART_GET_BYTE ();

      // Behaviour similar to Unix stty ICRNL.
      if ( ch == '\r' ) {
        ch = '\n';
      }
      if ( ch == '\n' ) {
        *cp = ch;
        term_io_putchar (ch, stream);
        rxp = buf;
        break;
      }
      else if ( ch == '\t' ) {
        ch = ' ';
      }

      if ( (ch >= (uint8_t)' ' && ch <= (uint8_t)'\x7e') ||
           ch >= (uint8_t)'\xa0') {
        if ( cp == buf + RX_BUFSIZE - 1 ) {
          term_io_putchar ('\a', stream);
        }
        else {
          *cp++ = ch;
          term_io_putchar (ch, stream);
        }
        continue;
      }

      switch ( ch ) {
        case 'c' & 0x1f:
          return -1;

        case '\b':
        case '\x7f':
          if (cp > buf) {
            term_io_putchar ('\b', stream);
            term_io_putchar (' ', stream);
            term_io_putchar ('\b', stream);
            cp--;
          }
          break;

        case 'r' & 0x1f:
          term_io_putchar ('\r', stream);
          for ( cp2 = buf; cp2 < cp; cp2++ ) {
            term_io_putchar ( *cp2, stream);
          }
          break;

        case 'u' & 0x1f:
          while (cp > buf) {
            term_io_putchar ('\b', stream);
            term_io_putchar (' ', stream);
            term_io_putchar ('\b', stream);
            cp--;
          }
          break;

        case 'w' & 0x1f:
          while (cp > buf && cp[-1] != ' ') {
            term_io_putchar ('\b', stream);
            term_io_putchar (' ', stream);
            term_io_putchar ('\b', stream);
            cp--;
          }
          break;
      }
    }
  }

  ch = *rxp++;

  if ( ch == '\n' ) {
    rxp = 0;
  }

  return ch;
}


int
term_io_getline (char *linebuf)
{
  char *lbp = linebuf;
    
  size_t char_count = 0;

  for ( ; ; ) {

    int ch = term_io_getchar (stdin);

    switch ( ch ) {
      case -1:
        return -1;
        break;
      case _FDEV_EOF:
        // FIXME: is there something intelligent we should do here?
      case '\n':
        // FIXME: we should do something intelligent when we're about to
        // overflow the buffer.  For now we just return -1, maybe we should
        // stuff an error message in the returned string?
        *lbp = ch;
        lbp++;
        char_count++;
        if ( char_count == TERM_IO_LINE_BUFFER_MIN_SIZE ) {
          return -1;
        }
        *lbp = '\0';
        return char_count;
        break;
      default:
        *lbp = ch;
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
