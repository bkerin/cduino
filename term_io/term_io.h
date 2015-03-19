// Terminal-style input and output (i.e. basic line editing and formatted
// output functionality) for the Arduino serial port.

// Test driver: term_io_test.c    Implementation: term_io.c

#ifndef TERM_IO_H
#define TERM_IO_H

#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>

#include "uart.h"

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
//     themselves provide any screen echo or command-line editing capability
//     (hence the term_io_getline() function).
//
//   * Some of the AVR libc output functions (printf() and friends) provide
//     different levels of functionality and libc-ishness depending on
//     the linker flags used; see AVRLIBC_PRINTF_LDFLAGS in generic.mk
//     for details.
void
term_io_init (void);

// The term_io_getline() function uses an internal buffer this big
#define TERM_IO_RX_BUFSIZE 81

// The buffer supplied to the term_io_getline() must be at least this big
#define TERM_IO_LINE_BUFFER_MIN_SIZE (TERM_IO_RX_BUFSIZE + 1)

// Get a line of input from the terminal, and save it in linebuf (which
// must be at least TERM_IO_LINE_BUFFER_MIN_SIZE bytes long).  Basic command
// line editing is available for the user entering the line; see uart.h for
// details.  Returns the number of characters retrieved (including trailing
// newline but not including trailing null), or -1 if an error occurs.
int
term_io_getline (char *linebuf);

// FIXME: This is intended as a version of printf_P() that tries to check
// the types of the variadic arguments as GCC normally does.  It could then
// be used in TERM_IO_PFP() etc.  However, there are some problems: one is
// that PSTR() doesn't do what doxygen says it does: the actualy pgmspace.h
// as of 1.8.0 explicitly does something different that what the doc version
// says, so if PSTR() is used for the format argument the detection doesn't
// happen.  Also, we haven't verified actual RAM savings with this wrapper.
// And finally, avr libc weirdly doesn't provide vprintf_P, though it does
// provide vfprintf_P and also vsprintf_P, which is theoretically not a
// problem but just weird.
//
// FIXME: it might be possible to turn printf_P into a do while ( 0 )
// type thing containing a declaration for the string to go in flash,
// since it looks like the reason PSTR() does goofy secret stuff is that
// progmem on a type isn't supported in GCC.  Or if that doesn't work or
// doesn't really save RAM I guess we could have one set of functions for
// (carefully checked) test program output and another for debug output,
// but god that's ugly and then people using the test as prototype would
// end up screwing themselves.
//
// FIXME: so the current situation is:
// this detects errors:
//   printf_P_safer ( ((const PROGMEM char *) "biggy: %lu\n"), 42);
// but might not actually save RAM, and this doesn't detect type errors:
//   printf_P_safer ( PSTR ("biggy: %lu\n"), 42);
//
//int
//printf_P_safer (char const *fmt, ...) __attribute__ ((format (printf, 1, 2)));

// PrintF using Program memory.  This macro makes it easier to store the
// format arguments to printf_P() calls in program space.
#define TERM_IO_PFP(format, ...) printf_P (PSTR (format), ## __VA_ARGS__)

// Print Trace Point message.  Useful for debugging.
#define TERM_IO_PTP()                                   \
  TERM_IO_PFP (                                         \
      "trace point: file %s, line %d, function %s()\n", \
      __FILE__, __LINE__, __func__ )

// Print Halt Point message and call exit(1).  Note that exit will disable
// all interrupts before entering an infinite loop.
#define TERM_IO_PHP()                                    \
  do {                                                   \
    TERM_IO_PFP (                                        \
        "halt point: file %s, line %d, function %s()\n", \
        __FILE__, __LINE__, __func__);                   \
    exit (1);                                            \
  } while ( 0 );

// This macro is supposed to Print a Failure Point message like assert()
// on a big computer.
#define TERM_IO_PFP_ASSERT(condition)                 \
  do {                                                \
    if ( UNLIKELY (! (condition)) ) {                 \
      PFP ("%s:%i: %s: Assertion `%s' failed.\n",     \
          __FILE__, __LINE__, __func__, #condition ); \
      assert (FALSE);                                 \
    }                                                 \
  } while ( 0 )

// Like TERM_IO_PFP_ASSERT(), but it trips whenever it's reached.
#define TERM_IO_PFP_ASSERT_NOT_REACHED()                               \
  do {                                                                 \
    PFP ("%s:%i: %s: Assertion failed: code should not be reached.\n", \
        __FILE__, __LINE__, __func__ );                                \
    assert (FALSE);                                                    \
  } while ( 0 )

// Assert that result is 0.  This macro is intended to make it easier to check
// the results of functions that return status codes where 0 means success and
// other values indicate errors or abnormal conditions.  The string_fetcher
// is a function that gets the string form of the enumerated value result,
// and string_buf is supposed to point to storage for that string.  See the
// use case (of the synonym PFP_ASSERT_SUCCESS()) in one_wire_master_test.c.
// This macro is guaranteed to evaluate it's result argument exactly once.
#define TERM_IO_PFP_ASSERT_SUCCESS(result, string_fetcher, string_buf) \
  do {                                                                 \
    int XxX_result = result;                                           \
    if ( UNLIKELY (XxX_result) ) {                                     \
      PFP (                                                            \
          "%s:%i: %s: failure: %s\n",                                  \
          __FILE__, __LINE__, __func__,                                \
          string_fetcher (XxX_result, string_buf) );                   \
      assert (FALSE);                                                  \
    }                                                                  \
  } while ( 0 )

// The whole point of the above macros is that they take some typing out of
// the edit-compile-debug cycle, so you may like to stick something like
// 'CPPFLAGS += -DTERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP' at the
// bottom of the Makefile for your module to make things even easier :)
#ifdef TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP

#  define PFP                    TERM_IO_PFP
#  define PTP                    TERM_IO_PTP
#  define PHP                    TERM_IO_PHP
#  define PFP_ASSERT             TERM_IO_PFP_ASSERT
#  define PFP_ASSERT_NOT_REACHED TERM_IO_PFP_ASSERT_NOT_REACHED
#  define PFP_ASSERT_SUCCESS     TERM_IO_PFP_ASSERT_SUCCESS

// Perhaps you even like to use lower case :)
#  define pfp                    TERM_IO_PFP
#  define ptp                    TERM_IO_PTP
#  define php                    TERM_IO_PHP
#  define pfp_assert             TERM_IO_PFP_ASSERT
#  define pfp_assert_not_reached TERM_IO_PFP_ASSERT_NOT_REACHED
#  define pfp_assert_success     TERM_IO_PFP_ASSERT_SUCCESS

#endif

#endif // TERM_IO_H
