// Interface to hardware serial port (UART) controller.
//
// Test driver: uart_test.c    Implementation: uart.c

#ifndef UART_H
#define UART_H

#include <avr/io.h>

// This module supports serial port initialization and byte transfer using
// polling (i.e. busy waits, not interrupts).

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

// Send a byte to the serial port
#define UART_PUT_BYTE(byte) \
  do { \
    loop_until_bit_is_set (UCSR0A, UDRE0); \
    UDR0 = byte; \
  } while ( 0 );

// Evaluate to true iff an incoming byte is ready to be read.  You should
// check for errors before actually reading it, since you can't do so
// afterwords.
#define UART_BYTE_AVAILABLE() (UCSR0A & _BV (RXC0))

// Block until a byte comes in from the serial port.  Note that this could
// block forever.
#define UART_WAIT_FOR_BYTE() loop_until_bit_is_set (UCSR0A, RXC0)

// This macro evaluates to true iff there is a receiver error flag set.
// This should be called immediately after UART_WIAT_FOR_BYTE() and before
// UART_GET_BYTE().  FIXXME: if we supported parity mode, we would need to
// check for UPE0 bit as well.
#define UART_RX_ERROR() (UCSR0A & (_BV (FE0) | _BV (DOR0)))

// These macros evaluate to true iff particular receiver error flags are set.
// They can be used after or instead of UART_RX_ERROR() to determine the
// detailed cause of the error.
#define UART_RX_FRAME_ERROR() (UCSR0A & _BV (FE0))
#define UART_RX_DATA_OVERRUN_ERROR() (UCSR0A & _BV (DOR0))

// Get the byte that is ready to be recieved.  This should only be used after
// UART_BYTE_AVAILABLE() evaluate to true or UART_WAIT_FOR_BYTE() completes.
// Using this macro clears the error flags underlying the UART_RX_ERROR_*()
// macros.
#define UART_GET_BYTE() UDR0

#endif // UART_H
