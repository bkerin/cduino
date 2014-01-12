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

#include <avr/io.h>

#include "uart.h"

void
uart_init (void)
{
  // FIXME: is there some hardware that we should be waking from sleep here?

#ifndef F_CPU
#  error the AVR libc util/setbaud.h header will require F_CPU to be defined
#endif

  // The magical AVR libc util/setbaud.h header requires BAUD to be defined,
  // so we first ensure it isn't already defined then set it.
#ifdef BAUD
#  error We need to set BAUD, but it's already defined, so we're too scared
#endif
#define BAUD UART_BAUD

// This is a special calculation-only header that can be included anywhere.
// Its going to give us back some macros that help set up the serial port
// control registers: UBRRH_VALUE, UBRRL_VALUE, and USE_2X.
#include <util/setbaud.h>

  // Set up clocking
  UBRR0L = UBRRL_VALUE;
  UBRR0H = UBRRH_VALUE;
#if USE_2X
  UCSR0A |= _BV (U2X0);
#else
  UCSR0A &= ~(_BV (U2X0));
#endif

  UCSR0B = _BV (TXEN0) | _BV (RXEN0);   // Enable TX/RX
}
