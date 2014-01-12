// Test/demo for the uart.h interface.

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo
 *
 * $Id: stdiodemo.c 1008 2005-12-28 21:38:59Z joerg_wunsch $
 */

// This program puts a prompt out to the serial port, reads some character
// from the serial port, then sends the read characters back out on the
// serial port.
//
// There are no external hardware requirements other than an arduino and
// a USB cable to connect it to the computer.

// FIXME: we don't need all these headers now
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include "uart.h"


int
main (void)
{
  uart_init ();

  assert (sizeof (char) == 1);   // Hey, I like probably correct programs :)

#define CHARS_TO_READ 5

  char ptec[] = "\n\rType some characters now\n\r";   // Prompt To Enter Chars
  char ce[CHARS_TO_READ];   // Characters entered

  for ( ; ; ) {

    // Put a prompt on the wire
    for ( uint8_t ii = 0 ; ii < sizeof (ptec) ; ii++ ) {
      UART_PUT_BYTE (ptec[ii]);
    }

    // Read the characters entered
    for ( uint8_t ii = 0 ; ii < CHARS_TO_READ ; ii++ ) {
      UART_WAIT_FOR_BYTE ();
      if ( UART_RX_ERROR () ) {
        // Hope we aren't here.  We could try to print an error...
        assert (0);
      }
      ce[ii] = UART_GET_BYTE ();
    }

    char ebp[] = "You entered these characters: ";   // Echo Back Prefix

    for ( uint8_t ii = 0 ; ii < sizeof (ebp) ; ii++ ) {
      UART_PUT_BYTE (ebp[ii]);
    }

    for ( uint8_t ii = 0 ; ii < CHARS_TO_READ ; ii++ ) {
      UART_PUT_BYTE (ce[ii]);
    }

    UART_PUT_BYTE ('\n');
  }
}
