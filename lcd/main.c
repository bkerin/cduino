/*
  LiquidCrystal Library - Hello World
 
 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the 
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.
 
 This sketch prints "Hello World!" to the LCD
 and shows the time.
 
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 
 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

#include <avr/pgmspace.h>
#include <string.h>
#include <util/delay.h>

// include the library code:
#include "lcd.h"

int
main (void)
{
  lcd_init ();

  // Set up the LCD's number of columns and rows: 
  lcd_begin (16, 2);

  // Print a message to the LCD.
  lcd_write_string ("hello, world!");

  double magic_number = 42.55;   // Something to output.

  double time_per_test_ms = 1000.0;   // Time we spend on most tests, in ms.

  // Set the cursor to column 0, line 1.  Note: line 1 is the second row,
  // since counting begins with 0.
  lcd_setCursor (0, 1);

  // Test lcd_printf().
  lcd_printf ("%.2f ", magic_number);
  _delay_ms (time_per_test_ms);

  // Test lcd_printf_P().
  lcd_printf_P (PSTR ("%.2f "), magic_number);
  _delay_ms (time_per_test_ms);

  // Test cursor on/off routines. 
  lcd_cursor ();   
  _delay_ms (time_per_test_ms);
  lcd_noCursor ();

  // Test blinking cursor on/off routines.
  lcd_blink ();
  _delay_ms (time_per_test_ms);
  lcd_noBlink ();

  // Test turning display off and on again.
  lcd_display_off ();
  _delay_ms (time_per_test_ms);
  lcd_display_on ();

  // Test setting the cursor somewhere exotic.
  uint8_t test_start_col = 12, test_start_row = 1;
  const char test_char = 'X';
  lcd_setCursor (test_start_col, test_start_row);
  lcd_write (test_char);
  _delay_ms (time_per_test_ms);

  lcd_clear ();

  // Test display scrolling. 
  test_start_col = 0, test_start_row = 0;
  lcd_setCursor(test_start_col, test_start_row);
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
}
