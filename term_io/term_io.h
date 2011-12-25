// Terminal-style input and output (i.e. basic line editing and formatted
// output functionality) for the ardiono serial port.

#ifndef TERM_IO_H
#define TERM_IO_H

#include "uart.h"

// FIXME: should this module or the uart module use the util/setbaud.h
// interface from AVR libc.

// Set up USART0 and AVR libc stdio interface such that printf() (and
// friends) can be used for output, and term_io_getline() used for input
// via a GNU screen session (with default settings) or something similar.
//
// Things to consider:
//
//   * Calling term_io_init() sets up the PD0 (RXD) and PD1 (TXD) pins such
//     that they cannot be used for normal digital IO.
//
//   * The AVR libc input functions (scanf() and friends) do not by
//     themselves provide provide any screen echo or command-line editing
//     capability (hence the term_io_getline() function).
//
//   * Some of the AVR libc output functions (printf() and friends) provide
//     different levels of functionality and libc-ishness depending on
//     the linker flags used; see AVRLIBC_PRINTF_LDFLAGS in generic.mk
//     for details.
void
term_io_init (void);

// The buffer used for the term_io_getline() must be at least this big.
#define TERM_IO_LINE_BUFFER_MIN_SIZE (RX_BUFSIZE + 1)
#define LINEBUFSIZE (RX_BUFSIZE + 1)  // Old name

// Get a line of input from the terminal, and save it in linebuf (which
// must be at least TERM_IO_LINE_BUFFER_MIN_SIZE bytes long).  Basic command
// line editing is available for the user entering the line; see uart.h for
// details.  Returns the number of characters retrieved (including trailing
// newline but not including trailing null), or -1 if an error occurs.
int
term_io_getline (char *linebuf);

#endif // TERM_IO_H
