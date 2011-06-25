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

#include <assert.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term_io.h"

#define SIG_LEN 4               // Length of our signature string
#define OFF_SIG ((uint32_t *) 0)   // Our signature offset
#define OFF_CTR ((uint8_t *) 4)    // Write counter offset
#define OFF_LEN ((uint8_t *) 5)    // String length offset
#define OFF_TXT ((uint8_t *) 6)    // String text offset

int
main (void)
{
  char *signature = "AVRm";
  char buffer[TERM_IO_LINE_BUFFER_MIN_SIZE];
  uint8_t write_counter = 0;
  uint8_t str_length;

  term_io_init ();

  // NOTE: you may have to connect to the AVR right after a reboot to see
  // this startup stuff happen...

  // Make sure the EEPROM is ready. 
  if ( !eeprom_is_ready () ) {
    printf_P (PSTR ("Waiting for EEPROM to become ready...\n"));
    eeprom_busy_wait ();
  }
  printf_P (PSTR ("EEPROM ready.\n"));

  // Check for signature, report what we find, and format if necessary.
  printf_P (PSTR ("Checking EEPROM format...\n"));
  eeprom_read_block (&buffer, OFF_SIG, 4);
  if ( memcmp (buffer, signature, SIG_LEN) == 0 ) {
    printf_P (PSTR ("EEPROM already formatted.\n\n"));
    write_counter = eeprom_read_byte (OFF_CTR);
  }
  else {
    printf_P (PSTR ("EEPROM is blank, formatting...\n"));
    eeprom_write_block (signature, OFF_SIG, 4);
    eeprom_write_byte (OFF_CTR, (int8_t) 0);
    eeprom_write_byte (OFF_LEN, (int8_t) 0);
    // Null byte at start of string effectively blanks the whole string.
    eeprom_write_byte (OFF_TXT, (int8_t) 0);  
    printf_P (PSTR ("EEPROM formatted.\n\n"));
  }

  while (1) {

    // Prompt to determine if we want to read or write EEPROM.
    printf_P (PSTR ("(writes: %d) [r]ead, [w]rite, [e]rase: "), write_counter);
    int char_count = term_io_getline (buffer);
    assert (char_count != -1);

    switch ( buffer[0] ) {

      case 'W':
      case 'w':
        printf_P (PSTR ("Enter a string to store in EEPROM: "));
        int char_count = term_io_getline (buffer);
        assert (char_count != -1);
        str_length = strlen (buffer);
        printf_P (PSTR ("EEPROM written.\n\n"));

        // Increment the counter.
        write_counter = eeprom_read_byte (OFF_CTR) + 1;
        eeprom_write_byte (OFF_CTR, write_counter);

        // Stash the length.
        eeprom_write_byte (OFF_LEN, str_length);

        // Write out the input text (+1 for trailing null byte).
        eeprom_write_block (&buffer, OFF_TXT, str_length + 1);
        break;

      case 'R':
      case 'r':
        write_counter = eeprom_read_byte (OFF_CTR);

        str_length = eeprom_read_byte (OFF_LEN);

        eeprom_read_block (&buffer, OFF_TXT, str_length + 1);
        printf_P (PSTR ("Contents of string in EEPROM: %s\n"), buffer);
        break;

      case 'E':
      case 'e':
        eeprom_write_dword (OFF_SIG, 0x0000);
        eeprom_write_byte (OFF_CTR, (int8_t) 0);
        eeprom_write_byte (OFF_LEN, (int8_t) 0);
        // Null byte at start of string effectively blanks the whole string.
        eeprom_write_byte (OFF_TXT, (int8_t) 0);
        write_counter = 0;
        printf_P (PSTR ("EEPROM erased.\n\n"));
        break;

      default:
        printf_P (PSTR ("Invalid operation (first letter not r,w, or e)\n"));
        break;
    }
  }
}
