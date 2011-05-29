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

#include "defines.h"

#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>

#include "uart.h"

void
uart_init (void)
{
#if F_CPU < 2000000UL && defined(U2X)
  UCSR0A = _BV(U2X0);   // Improve baud rate error by using 2x clk.
  UBRR0L = (F_CPU / (8UL * UART_BAUD)) - 1;
#else
  UBRR0L = (F_CPU / (16UL * UART_BAUD)) - 1;
#endif
  UCSR0B = _BV(TXEN0) | _BV(RXEN0);   // Enable TX/RX.
}

int
uart_putchar (char c, FILE *stream)
{
  if ( c == '\a' ) {
    fputs ("*ring*\n", stderr);
    return 0;
  }

  if ( c == '\n' ) {
    uart_putchar ('\r', stream);
  }
  loop_until_bit_is_set (UCSR0A, UDRE0);
  UDR0 = c;

  return 0;
}

int
uart_getchar (FILE *stream)
{
  uint8_t c;
  char *cp, *cp2;
  static char b[RX_BUFSIZE];
  static char *rxp;

  if ( rxp == 0 ) {
    for ( cp = b ; ; ) {
      loop_until_bit_is_set (UCSR0A, RXC0);
      if ( UCSR0A & _BV(FE0) ) {
        return _FDEV_EOF;
      }
      if ( UCSR0A & _BV(DOR0) ) {
        return _FDEV_ERR;
      }
      c = UDR0;

      // Behaviour similar to Unix stty ICRNL.
      if ( c == '\r' ) {
        c = '\n';
      }
      if ( c == '\n' ) {
        *cp = c;
        uart_putchar (c, stream);
        rxp = b;
        break;
      }
      else if ( c == '\t' ) {
        c = ' ';
      }

      if ( (c >= (uint8_t)' ' && c <= (uint8_t)'\x7e') ||
           c >= (uint8_t)'\xa0') {
        if ( cp == b + RX_BUFSIZE - 1 ) {
          uart_putchar ('\a', stream);
        }
        else {
          *cp++ = c;
          uart_putchar (c, stream);
        }
        continue;
      }

      switch ( c ) {
        case 'c' & 0x1f:
          return -1;

        case '\b':
        case '\x7f':
          if (cp > b) {
            uart_putchar ('\b', stream);
            uart_putchar (' ', stream);
            uart_putchar ('\b', stream);
            cp--;
          }
          break;

        case 'r' & 0x1f:
          uart_putchar ('\r', stream);
          for ( cp2 = b; cp2 < cp; cp2++ ) {
            uart_putchar ( *cp2, stream);
          }
          break;

        case 'u' & 0x1f:
          while (cp > b) {
            uart_putchar ('\b', stream);
            uart_putchar (' ', stream);
            uart_putchar ('\b', stream);
            cp--;
          }
          break;

        case 'w' & 0x1f:
          while (cp > b && cp[-1] != ' ') {
            uart_putchar ('\b', stream);
            uart_putchar (' ', stream);
            uart_putchar ('\b', stream);
            cp--;
          }
          break;
      }
    }
  }

  c = *rxp++;

  if ( c == '\n' ) {
    rxp = 0;
  }

  return c;
}

