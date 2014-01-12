// Implementation of the interface described in term_io.h.

#include <stdio.h>

#include "term_io.h"
#include "uart.h"

static int
term_io_putchar (char ch, FILE *stream)
{
  // Wierdo routine.  Satisfies avrlibc's requirements for a stream
  // implementation function.  This routine first substitutes any given
  // newline with a carriage return (i.e changes '\n' to '\r') then puts
  // the resulting character out on the serial port using UART_PUT_BYTE().

  if ( ch == '\n' ) {
    // I think we could just be putting this byte out directly (our steam
    // in this case is tied inevitably to the serial port).
    term_io_putchar ('\r', stream);
  }

  UART_PUT_BYTE (ch);

  return 0;
}

static int
term_io_getchar (FILE *stream)
{
  // Wierdo routine.  // Satisfies avrlibc's requirements for a stream
  // implementation function.  Its actually line buffered, might propagate
  // errors upwards when trying to receive bytes from the serial port,
  // and might term_io_putchar() bytes in response to the byte it reads.
  // Its doing all this crazy stuff in order to help us get somewhat
  // terminal-like command line editing.  In more detail:
  //
  // This features a simple line-editor that allows to delete and re-edit the
  // characters entered, until either CR or NL is entered.  Printable characters
  // entered will be echoed using uart_putchar().
  //
  // Editing characters:
  //
  //   \b (BS) or \177 (DEL)    delete the previous character
  //   ^u                       kills the entire input buffer
  //   ^w                       deletes the previous word
  //   ^r                       sends a CR, and then reprints the buffer
  //   \t                       will be replaced by a single space
  //
  // All other control characters will be ignored.
  //
  // The internal line buffer is TERM_IO_RX_BUFSIZE characters long, which
  // includes the terminating \n (but no terminating \0).  If the buffer
  // is full (i. e., at TERM_IO_RX_BUFSIZE - 1 characters in order to keep
  // space for the trailing \n), any further input attempts will send a \a to
  // uart_putchar() (BEL character), although line editing is still allowed.
  //
  // Input errors while talking to the UART will cause an immediate return of
  // -1 (error indication).  Notably, this will be caused by a framing error
  // (e. g.  serial line "break" condition), by an input overrun, and by a
  // parity error (if parity was enabled and automatic parity recognition
  // is supported by hardware).
  //
  // Successive calls to uart_getchar() will be satisfied from the internal
  // buffer until that buffer is emptied again.

  uint8_t ch;
  char *cp, *cp2;
  static char buf[TERM_IO_RX_BUFSIZE];
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
        if ( cp == buf + TERM_IO_RX_BUFSIZE - 1 ) {
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

        // FIXXME: it would be nice to say a few words about these magic
        // values (e.g. '\x7f').

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

static FILE term_io_str
  = FDEV_SETUP_STREAM (term_io_putchar, term_io_getchar, _FDEV_SETUP_RW);

void
term_io_init (void)
{
  uart_init ();

  stdout = stdin = &term_io_str;
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
