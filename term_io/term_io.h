// Terminal-style input and output (i.e. basic line editing and formatted
// output functionality) for the ardiono serial port.

#ifndef TERM_IO_H
#define TERM_IO_H

#include "uart.h"

// FIXME: should this module or the uart module use the util/setbaud.h
// interface from AVR libc.

// Set up the uart and AVR libc stdio interface such that printf() (and
// friends) can be used for output, and term_io_getline() used for input
// via a GNU screen session (with default settings) or something similar.
// Note that the AVR libc input functions (scanf() and friends) do not by
// themselves provide any screen echo or command-line editing capability
// (hence the term_io_getline() function).  Note also that some of the AVR
// libc output functions (printf() and friends) profide different levels
// of functionality and libc-ishness depending on the linker flags used;
// see AVRLIBC_PRINTF_LDFLAGS in generic.mk for details.
void
term_io_init (void);

// The buffer used for the term_io_getline() must be at least this big.
#define LINEBUFSIZE (RX_BUFSIZE + 1)

// Get a line of input from the terminal, and save it in linebuf (which
// must be at least LINEBUFSIZE bytes long).  Basic command line editing
// is available for the user entering the line; see uart.h for details.
// Returns the number of characters retrieved (including trailing newline
// but not including trailing null), or -1 if an error occurs.
int
term_io_getline (char *linebuf);

#endif // TERM_IO_H
