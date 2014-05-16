// Interface to hardware serial port (UART) controller.
//
// Test driver: uart_test.c    Implementation: uart.c
//
// This module supports serial port initialization and byte transfer
// using polling (i.e. busy waits, not interrupts).  Only the core UART
// functionality is implemented here, not all the serial bells and whistles
// (i.e. no CTS/RTS or other extra serial port signals).

#ifndef UART_H
#define UART_H

#include <avr/io.h>

// F_CPU is supposed to be defined in the Makefile (because that's where
// the other part and programmer specs go).
#ifndef F_CPU
#  error "F_CPU not defined"
#endif

#define UART_BAUD 9600

// Initialize the USART0 to 9600 Bd, TX/RX, 8N1.  Note that this sets up
// the PD0 (RXD) and PD1 (TXD) pins such that they cannot be used for normal
// digital IO.  The ATMega328P datasheet says that SART0 must be reinitialized
// after waking from sleep.  In practive I haven't found it to need this, but
// this function is guaranteed to be callable in this situation just in case.
void
uart_init (void);

// Send a byte to the serial port
#define UART_PUT_BYTE(byte) \
  do { \
    loop_until_bit_is_set (UCSR0A, UDRE0); \
    UDR0 = (byte); \
  } while ( 0 );

// Evaluate to true iff an incoming byte is ready to be read.  You should
// check for errors before actually reading it, since you can't do so
// afterwords.
#define UART_BYTE_AVAILABLE() (UCSR0A & _BV (RXC0))

// Block until a byte comes in from the serial port.  Note that this could
// block forever.
#define UART_WAIT_FOR_BYTE() loop_until_bit_is_set (UCSR0A, RXC0)

// This macro evaluates to true iff there is a receiver error flag set.
// This should be called immediately after UART_BYTE_AVAILABLE() or
// UART_WIAT_FOR_BYTE() and before UART_GET_BYTE().  When a receiver
// error is detected, the receive buffer should be flushed using
// UART_FLUSH_RX_BUFFER() before any other attempt to use the UART (except
// for the UART_RX_FRAME_ERROR() and UART_RX_DATA_OVERRUN_ERROR() macros).
// Flushing the receive buffer clears the error flags (rendering error details
// unrecoverable).  Not clearing the flag can result in confusing errors
// later, and/or endless failure of other function that try to read data.
// FIXXME: if we supported parity mode, we would need to check for the UPE0
// bit as well.
#define UART_RX_ERROR() (UCSR0A & (_BV (FE0) | _BV (DOR0)))

// This macro evaluates to true when the frame error flag is set (indicating
// that a stop bit failed to be 1).
#define UART_RX_FRAME_ERROR() (UCSR0A & _BV (FE0))

// This macro evaluates to true when the receiver buffer overflow flag it set.
// The receiver buffer is only two bytes deep, so this can easily occur if
// you don't poll the serial port fast enough.
#define UART_RX_DATA_OVERRUN_ERROR() (UCSR0A & _BV (DOR0))

// Get the byte that is ready to be recieved.  This should only be used after
// UART_BYTE_AVAILABLE() evaluates to true or UART_WAIT_FOR_BYTE() completes.
// Using this macro will probably clear the error flags underlying the
// UART_RX_ERROR_*() macros, but UART_FLUSH_RX_BUFFER() is the certain way
// to do that.
#define UART_GET_BYTE() UDR0

// Flush the receive buffer.  This should be called after a receiver error
// has occurred.
#define UART_FLUSH_RX_BUFFER() \
  do { \
    uint8_t XxX_dummy; \
    while ( UART_BYTE_AVAILABLE() ) { \
      XxX_dummy = UART_GET_BYTE (); \
    } \
  } while ( 0 );

#endif // UART_H
