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

// This program shows how to set up an AVR libc FILE stream to communicate with
// the microcontroller over the serial line.  The program accepts commands of
// the form 'u some_string' and simply returns the string.

#include "defines.h"

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

/*
 * Do all the startup-time peripheral initializations.
 */
static void
ioinit(void)
{
  uart_init();
}

FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

int
main(void)
{
  char buf[20], s[20];

  ioinit();

  stdout = stdin = &uart_str;

  for (;;)
    {
      printf_P(PSTR("\nAVR Ready.\nEnter command: "));
      if (fgets(buf, sizeof buf - 1, stdin) == NULL)
	break;
      if (tolower(buf[0]) == 'q')
	break;

      switch (tolower(buf[0]))
	{
	default:
	  printf("Unknown command: '%c'\n", buf[0]);
	  break;

        case '\n':
          break;

	case 'l':
	  if (sscanf(buf, "%*s %s", s) > 0)
	    {
	      printf("AAAACCCckkkk we can't do that (LCD unimplemented)\n");
	    }
	  else
	    {
	      printf("sscanf() failed\n");
	    }
	  break;

	case 'u':
	  if (sscanf(buf, "%*s %s", s) > 0)
	    {
	      fprintf(&uart_str, "Got %s\n", s);
	      printf("OK\n");
	    }
	  else
	    {
              if (sscanf(buf, "u ") == 0) {
                fprintf(&uart_str, "Got 'u' command without an argument\n");
              } 
              else {
	        printf("sscanf() failed\n");
              }
	    }
	  break;
	}
    }

  return 0;
}
