// Exercise the interface described in lcd.h.  
//
// This test program requires an Hitachi HD44780 LCD display to be connected
// using the pin connections defined in the Makefile, and a couple of other
// LCD pins to be connected in particular ways to make the LCD usable.
//
// Unless something has changed in the Makefile, the LCD pin connections
// should be:
//
//   * LCD RS pin to digital pin 8
//   * LCD Enable pin to digital pin 9
//   * LCD D4 pin to digital pin 11
//   * LCD D5 pin to digital pin 12
//   * LCD D6 pin to digital pin 13
//   * LCD D7 pin to digital pin 14
//   * LCD R/W pin to ground
//   * 10K potentiometer:
//     * Ends to +5V and ground
//     * Wiper to LCD VO pin (pin 3).
//
// This example code is in the public domain.  It was created by
// David A. Mellis.  It was subsequently modified by Limor Fried
// (http://www.ladyada.net), then by Tom Igoe, and finally by Britton Kerin.

#include <avr/pgmspace.h>
#include <string.h>
#include <util/delay.h>

// include the library code:
#include "lcd.h"

int
main (void)
{
  lcd_init ();

  // Print a message to the LCD.
  lcd_write_string ("hello, world!");

  double magic_number = 42.64;   // Something to output.

  double time_per_test_ms = 1000.0;   // Time we spend on most tests, in ms.

  // Set the cursor to column 0, line 1.  Note: line 1 is the second row,
  // since counting begins with 0.
  lcd_set_cursor_position (0, 1);

  // Test lcd_printf().
  lcd_printf ("%.2f ", magic_number);
  _delay_ms (time_per_test_ms);

  // Test lcd_printf_P().
  lcd_printf_P (PSTR ("%.2f "), magic_number);
  _delay_ms (time_per_test_ms);

  // Test the underline cursor on/off routines. 
  lcd_underline_cursor_on ();   
  _delay_ms (time_per_test_ms);
  lcd_underline_cursor_off ();

  // Test blinking cursor on/off routines.
  lcd_blinking_cursor_on ();
  _delay_ms (time_per_test_ms);
  lcd_blinking_cursor_off ();

  // Test turning display off and on again.
  lcd_display_off ();
  _delay_ms (time_per_test_ms);
  lcd_display_on ();

  // Test setting the cursor somewhere exotic.
  uint8_t test_start_col = 12, test_start_row = 1;
  const char test_char = 'X';
  lcd_set_cursor_position (test_start_col, test_start_row);
  lcd_write (test_char);
  _delay_ms (time_per_test_ms);

  lcd_clear ();

  // Test display scrolling. 
  test_start_col = 0, test_start_row = 0;
  lcd_set_cursor_position(test_start_col, test_start_row);
  lcd_printf ("hello, big world!"); 
  const int chars_to_scroll = 3;
  const float ms_per_scroll_step = 500.0;
  for ( int ii = 0 ; ii < chars_to_scroll ; ii++ ) {
    lcd_scroll_left ();
    _delay_ms (ms_per_scroll_step);
  } 
  for ( int ii = 0 ; ii < chars_to_scroll ; ii++ ) {
    lcd_scroll_right ();
    _delay_ms (ms_per_scroll_step);
  } 

  // Test output of a couple of useful non-ASCII characters.  These may fail
  // depending on LCD model; see notes in the header file.
  lcd_home ();
  lcd_write (LCD_CHARACTER_RIGHT_ARROW);
  lcd_write (LCD_CHARACTER_LEFT_ARROW);
}
