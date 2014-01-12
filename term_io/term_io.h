// Terminal-style input and output (i.e. basic line editing and formatted
// output functionality) for the Arduino serial port.

// Test driver: term_io_test.c    Implementation: term_io.c

#ifndef TERM_IO_H
#define TERM_IO_H

#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>

#include "uart.h"

// FIXME: should this module or the uart module use the util/setbaud.h
// interface from AVR libc?

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

// PrintF using Program memory.  This macro makes it easier to store the
// format arguments to printf_P calls in program space.
#ifndef __GNUC__
#  error GNU C is required by a nearby comma-swallowing macro
#endif
#define TERM_IO_PFP(format, ...) printf_P (PSTR (format), ## __VA_ARGS__)

// Print Trace Point message.  Useful for debugging.
#define TERM_IO_PTP() \
  TERM_IO_PFP ( \
      "trace point: file %s, line %d, function %s()\n", \
      __FILE__, __LINE__, __func__ )

// Print Halt Point message and call exit(1).  Note that exit will disable
// all interrupts before entering an infinite loop.
#define TERM_IO_PHP() \
  do { \
    TERM_IO_PFP ( \
        "halt point: file %s, line %d, function %s()\n", \
        __FILE__, __LINE__, __func__); \
    exit (1); \
  } while ( 0 );

// The whole point of the above macros is that they take some typing out of
// the edit-compile-debug cycle, so you may like to stick something like
// 'CPPFLAGS += -DTERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP' at the
// bottom of the Makefile for your module to make things even easier :)
#ifdef TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#  define PFP TERM_IO_PFP
#  define PTP TERM_IO_PTP
#  define PHP TERM_IO_PHP
// Perhaps you even like to use lower case :)
#  define pfp TERM_IO_PFP
#  define ptp TERM_IO_PTP
#  define php TERM_IO_PHP
#endif

// Character-at-a-time interface.  Use the higher level printf() and
// term_io_getline() functions instead of this.  This is really only
// exposed so we can test it easily.  This routine first substitutes any
// given newline with a carriage return (i.e changes '\n' to '\r') then
// puts the resulting character out on the serial port using UART_PUT_BYTE().
int
term_io_putchar (char ch, FILE *stream);

// Character-at-a-time interface, sort of (see below).  Use the higher-level
// printf() and term_io_getline() instead of this.  This is really only
// exposed so we cat test it easily.  This routine first tries to get a
// single character from the serial port, possibly returning _FDEV_EOF if
// it fails due to a frame error or _FDEV_ERR if it fails due to a data
// overrun error.  It then performs a series of strange magical actions,
// which can include outputting multiple characters to the serial port, in
// order to get behavior similar to Unix stty.  The actual implementation
// is line-buffered.  See the implementation in term_io.c for more details.
int
term_io_getchar (FILE *stream);

#endif // TERM_IO_H
