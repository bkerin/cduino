// Test/demo for the lcd_keypad.h interface.
//
// Of course, this requires an installed DFRobot DFR0009 shield or equivalent.

// vim: foldmethod=marker

#include <assert.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
// FIXME: here only cause assert.h wrongly needs, remove when that bug is
// fixed (which it is in most recent upstream AVR libc
#include <stdlib.h>   
#include <string.h>
#include <util/delay.h>

#include "lcd_keypad.h"

int
main (void)
{
  lcd_keypad_init ();
  
  const double transition_message_time_ms = 2000.0;

  // Test lcd_keypad_wait_for_button() and lcd_keypad_button_name().
  // {{{1

  const uint8_t presses_required = 20;

  assert (sizeof (unsigned char) == 1);   // Hey, you never know :)
  lcd_printf ("Press %hhu buttons", presses_required);

  uint8_t presses = 0;
  while ( presses < presses_required ) {
    lcd_keypad_button_t button = lcd_keypad_wait_for_button ();
    presses++;

    lcd_clear ();
    lcd_home ();
    lcd_printf ("Press %hhu buttons", presses_required - presses);
    char button_name[LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1];
    lcd_keypad_button_name (button, button_name);
    lcd_set_cursor_position (0, 1);
    lcd_write_string (button_name);
  }
 
  lcd_clear ();
  lcd_home (); 
  lcd_write_string ("Ok, good enough");
  _delay_ms (transition_message_time_ms);

  // }}}1

  // Test lcd_keypad_show_value().
  // {{{1

  lcd_clear ();
  lcd_home ();
  lcd_write_string ("Will now test");
  lcd_set_cursor_position (0, 1);
  lcd_write_string ("show_value");
  _delay_ms (transition_message_time_ms);
  double the_answer = 42.0;
  lcd_keypad_button_t button
    = lcd_keypad_show_value ("the_answer", &the_answer);

  lcd_clear ();
  lcd_home ();
  lcd_write_string ("Finish button:");
  char button_name[LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1];
  lcd_keypad_button_name (button, button_name);
  lcd_set_cursor_position (0, 1);
  lcd_write_string (button_name);
  _delay_ms (transition_message_time_ms);

  // }}}1

  // Test lcd_keypad_set_value().
  // {{{1

  lcd_clear ();
  lcd_home (); 
  lcd_write_string ("Will now test");
  lcd_set_cursor_position (0, 1);
  lcd_write_string ("set_value");
  _delay_ms (transition_message_time_ms);
  the_answer = 42.0e-12;
  double step_size = 42e-13;
  button = lcd_keypad_set_value ("the_answer", &the_answer, step_size);

  lcd_clear ();
  lcd_home ();
  lcd_write_string ("Final answer:");
  lcd_set_cursor_position (0, 1);
  lcd_printf_P (PSTR (LCD_KEYPAD_VALUE_DISPLAY_FORMAT), the_answer);
  _delay_ms (transition_message_time_ms);

  lcd_clear ();
  lcd_home ();
  lcd_write_string ("Finish button:");
  lcd_keypad_button_name (button, button_name);
  lcd_set_cursor_position (0, 1);
  lcd_write_string (button_name);
  _delay_ms (transition_message_time_ms);
  
  // }}}1

  return 0;
}
