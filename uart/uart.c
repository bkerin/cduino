// Implementation of the interface described in uart.h.

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART implementation of interface described in uart.h
 *
 * $Id: uart.c 1008 2005-12-28 21:38:59Z joerg_wunsch $
 */

// See the interface description in uart.h for details of otherwise uncommented
// functions.

#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>

#include "uart.h"

void
uart_init (void)
{
  // FIXME: is there some hardware that we should be waking from sleep here?

  // Set up clocking.
#if F_CPU < 2000000UL && defined(U2X)
  UCSR0A = _BV(U2X0);   // Improve baud rate error by using 2x clk
  UBRR0L = (F_CPU / (8UL * UART_BAUD)) - 1;
#else
  UBRR0L = (F_CPU / (16UL * UART_BAUD)) - 1;
#endif

  UCSR0B = _BV(TXEN0) | _BV(RXEN0);   // Enable TX/RX.
}
