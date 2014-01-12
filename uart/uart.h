// Interface to hardware serial port (UART) controller.
//
// Test driver: uart_test.c    Implementation: uart.c

#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <stdio.h>

// The normal use pattern for this module looks something like this:
//
// uart_init ();
// uint8_t some_byte;
// UART_PUT_BYTE (some_byte);
// UART_WAIT_FOR_BYTE ();
// if ( UART_RX_ERROR () ) {
//   if ( UART_RX_FRAME_ERROR () ) {
//     // Do something?
//   } 
//   if ( UART_RX_DATA_OVERRUN_ERROR () ) {
//     // Do something?
//   }
// }
// some_byte = UART_GET_BYTE ();

// F_CPU is supposed to be defined in the Makefile (because that's where
// the other part and programmer specs go).
#ifndef F_CPU
#  error "F_CPU not defined"
#endif

#define UART_BAUD 9600

// Initialize the USART0 to 9600 Bd, TX/RX, 8N1.  Note that this sets up
// the PD0 (RXD) and PD1 (TXD) pins such that they cannot be used for normal
// digital IO.
void
uart_init (void);

#define UART_PUT_BYTE(byte) \
  do { \
    loop_until_bit_is_set (UCSR0A, UDRE0); \
    UDR0 = byte; \
  } while ( 0 );

// Block until a byte comes in from the serial port.  FIXME: would a
// timeout-or-iteration-limmited version of this be useful?
#define UART_WAIT_FOR_BYTE() loop_until_bit_is_set (UCSR0A, RXC0)

// This macro evaluates to true iff there is a receiver error flag set.
// This should be called immediately after UART_WIAT_FOR_BYTE() and before
// UART_GET_BYTE().  FIXXME: if we supported parity mode, we would need to
// check for UPE0 bit as well.
#define UART_RX_ERROR() (UCSR0A & (_BV (FE0) | _BV (DOR0)))

// These macros evaluate to true iff particular receiver error flags are set.
// They can be used if UART_RX_ERROR() evaluate to true to determine the
// cause of the error.
#define UART_RX_FRAME_ERROR() (UCSR0A & _BV (FE0))
#define UART_RX_DATA_OVERRUN_ERROR() (UCSR0A & _BV (DOR0))

// Get the byte that is ready to be recieved.  This should only be used
// after UART_WAIT_FOR_BYTE().  Using this macro clears the error flags
// underlying the UART_RX_ERROR_*() macros.
#define UART_GET_BYTE() UDR0

// First, if c is newline ('\n'), turn our copy of it it into a carriage
// return ('\r').  Then wait until the UART goes ready (UDRE0 bit of UCSR0A
// register set.  Then send the new character down the UART Tx.
// FIXME: the putchar-type stuff should go in term_io.h, not here.
int
uart_putchar (char c, FILE *stream);

// Size of internal line buffer used by uart_getchar().
#define RX_BUFSIZE 81

// Receive one character from the UART.  The actual reception is line-buffered,
// and one character is returned from the buffer at each invokation.
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
// The internal line buffer is RX_BUFSIZE characters long, which includes the
// terminating \n (but no terminating \0).  If the buffer is full (i. e., at
// RX_BUFSIZE - 1 characters in order to keep space for the trailing \n), any
// further input attempts will send a \a to uart_putchar() (BEL character),
// although line editing is still allowed.
//
// Input errors while talking to the UART will cause an immediate return of -1
// (error indication).  Notably, this will be caused by a framing error (e. g.
// serial line "break" condition), by an input overrun, and by a parity error
// (if parity was enabled and automatic parity recognition is supported by
// hardware).
//
// Successive calls to uart_getchar() will be satisfied from the internal
// buffer until that buffer is emptied again.
// FIXME: the putchar-type stuff should go in term_io.h, not here.
int
uart_getchar (FILE *stream);

#endif // UART_H
