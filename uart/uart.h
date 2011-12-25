/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART declarations
 *
 * $Id: uart.h 1008 2005-12-28 21:38:59Z joerg_wunsch $
 */

#ifndef UART_H
#define UART_H

#include <stdio.h>

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

// Send character c down the UART Tx, wait until tx holding register is empty.
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
int
uart_getchar (FILE *stream);

#endif // UART_H
