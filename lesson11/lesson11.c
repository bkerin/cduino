/* $CSK: lesson11.c,v 1.2 2009/05/17 20:21:23 ckuethe Exp $ */
/*
 * Copyright (c) 2009 Chris Kuethe <chris.kuethe@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

// Let the compiler do some of this, to avoid malloc.
static int cput(char, FILE *);
static int cget(FILE *);
static FILE O = FDEV_SETUP_STREAM(cput, NULL, _FDEV_SETUP_WRITE);
static FILE I = FDEV_SETUP_STREAM(NULL, cget, _FDEV_SETUP_READ );

static int
cput (char c, FILE *f)
{
  if ( c == '\n' ) {
    cput ((char) '\r', f);
  }

  loop_until_bit_is_set (UCSR0A, UDRE0);
  UDR0 = c;

  return 0;
}

static int
cget (FILE *f __attribute__((unused)))
{
	loop_until_bit_is_set (UCSR0A, RXC0);
	return UDR0;
}

#define OFF_SIG ((uint32_t *) 0)
#define OFF_CTR ((uint8_t *) 4)
#define OFF_LEN ((uint8_t *) 5)
#define OFF_TXT ((uint8_t *) 6)

int
main (void)
{
  char s[80], a[5], *sig = "AVRm";
  uint8_t b = 0, l = 0;

  // Set up stdio.
#define BAUD 9600
#include <util/setbaud.h>
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);
  stdout = &O;
  stdin  = &I;

  printf_P (PSTR ("eeprom test\n"));
  if ( !eeprom_is_ready () ) {
    printf_P(PSTR("waiting for eeprom to become ready\n"));
    eeprom_busy_wait();
  }
  printf_P(PSTR("eeprom ready\n"));

  // Check for signature.
  eeprom_read_block(&a, OFF_SIG, 4);
  if ( memcmp (a, sig, 4) == 0 ) {
    printf_P (PSTR ("eeprom formatted\n"));
    b = eeprom_read_byte (OFF_CTR);
  }
  else {
    printf_P (PSTR ("eeprom blanked\n"));
  }
  eeprom_write_block (sig, OFF_SIG, 4);

  while (1) {
    printf_P (PSTR ("(%d) [r]ead or [w]rite: "), b);
    scanf ("%3s", s);
    switch ( s[0] ) {
      case 'W':
      case 'w':
        printf_P (PSTR ("enter a string to store in eeprom\n>"));
        scanf ("%s", s);
        l = strlen (s);

        // Increment the counter.
        eeprom_busy_wait ();
        b = eeprom_read_byte(OFF_CTR) + 1;
        eeprom_busy_wait ();
        eeprom_write_byte(OFF_CTR, b);

        // Stash the length.
        eeprom_busy_wait ();
        eeprom_write_byte(OFF_LEN, l);

        // Write out the input text.
        eeprom_busy_wait ();
        eeprom_write_block (&s, OFF_TXT, l);
        break;
      case 'R':
      case 'r':
        eeprom_busy_wait ();
        b = eeprom_read_byte (OFF_CTR);

        eeprom_busy_wait ();
        l = eeprom_read_byte (OFF_LEN);

        eeprom_busy_wait ();
        eeprom_read_block (&s, OFF_TXT, l);
        printf_P (PSTR ("contents of eeprom\n>%s\n"), s);
        break;
      default:
        printf_P (PSTR ("invalid operation\n"));
    }
  }

  return 0;
}
