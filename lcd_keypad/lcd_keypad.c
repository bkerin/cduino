// Implementation of the interface described in lcd_keypad.h.

#include <assert.h>
// FIXME: here only cause assert.h wrongly needs, remove when that bug is
// fixed (which it is in most recent upstream AVR libc
#include <stdlib.h>   
#include <string.h>
#include <util/delay.h>

#include "adc.h"
#include "lcd.h"
#include "lcd_keypad.h"

void
lcd_keypad_init ()
{
  lcd_init ();
  adc_init (ADC_REFERENCE_AVCC);
}

void
lcd_keypad_button_name (lcd_keypad_button_t button, char *name)
{
  switch ( button ) {
    case LCD_KEYPAD_BUTTON_RIGHT:
      strncpy (name, "RIGHT", LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1);
      break;
    case LCD_KEYPAD_BUTTON_UP:
      strncpy (name, "UP", LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1);
      break;
    case LCD_KEYPAD_BUTTON_DOWN:
      strncpy (name, "DOWN", LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1);
      break;
    case LCD_KEYPAD_BUTTON_LEFT:
      strncpy (name, "LEFT", LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1);
      break;
    case LCD_KEYPAD_BUTTON_SELECT:
      strncpy (name, "SELECT", LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1);
      break;
    case LCD_KEYPAD_BUTTON_NONE:
      strncpy (name, "NONE", LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1);
      break;
    case LCD_KEYPAD_BUTTON_INDETERMINATE:
      strncpy (name, "INDETERMINATE", LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1);
      break;
    default:
      assert (0);   // Shouldn't be here.
      break;
    }
}


// FIXME: this poll interval is probably way too short for this purpose,
// think about/test it and review.  Actually, it might be kind of decent,
// because back when check buttons was returning multiple pushes for a single
// physical push when called in a loop, a super quick jab of one button got
// me 2 pushes returned.  Which doesn't say much besides being on the same
// order of timing, sort of.  Doesn't say much about the relative speed of
// transients accross the resistors.
static const double poll_interval_us = 100; 
 
#define BUTTON_COUNT 5
  
// FIXME: maybe this should be an exported constant or macro of the ADC module?
static const uint16_t a2d_steps = 1024;

static lcd_keypad_button_t
button_band (uint16_t raw_adc_reading)
{
  // The raw readings we expect to get from the ADC when different buttons
  // (or no button) are depressed, in the order in which lcd_keypad_button_t
  // enumeration values are listed in lcd_keypad.h.  These values are the
  // results of simple voltage divider calculations given the resistor
  // values specified in dfrobot-lcd-keypad-shield-schematic.pdf, and the
  // 0-1023 range of raw values returned by the ADC.  Of course, the actual
  // values may be different due to resistor tolerances or ADC error, so
  // we're really just curious which is closest.
  const uint16_t button_adc_center_values[BUTTON_COUNT + 1]
    = { 0, 144, 329, 505, 741, 1023 };

  lcd_keypad_button_t nearest_button;
  uint16_t min_delta = UINT16_MAX;
  uint8_t ii;
  for ( ii = 0 ; ii < BUTTON_COUNT + 1 ; ii++ ) {
    uint16_t current_delta
      = abs (button_adc_center_values[ii] - raw_adc_reading);
    if ( current_delta < min_delta ) {
      min_delta = current_delta;
      nearest_button = ii;
    }
  }

  return nearest_button;
}

lcd_keypad_button_t
lcd_keypad_check_buttons (void)
{
  // We require two ADC readings in the same band before we consider that we
  // have a definite button press at that value.  FIXME: this requires more
  // thought and a bit of research to determine how best to avoid picking
  // up spurious readings.
  uint16_t reading1, reading2;
  lcd_keypad_button_t band1, band2;
  reading1 = adc_read_raw (0);
  _delay_us (poll_interval_us);  
  reading2 = adc_read_raw (0);
  band1 = button_band (reading1);
  band2 = button_band (reading2);
 
  // Uncomment these lines to take a look at the raw reading values returned
  // by the ADC.  Note that you only get to see the readings for the buttons
  // while the button is held down, since afterwords the value reverts to
  // the raw reading corresponding to LCD_KEYPAD_BUTTON_NONE "button".
  //lcd_home ();
  //lcd_printf ("%4hu %4hu ", reading1, reading2);

  if ( band1 == band2 ) {
    return band1;
  }
  else {
    return LCD_KEYPAD_BUTTON_INDETERMINATE;
  }
}

lcd_keypad_button_t
lcd_keypad_wait_for_button (void)
{
  lcd_keypad_button_t result_button = LCD_KEYPAD_BUTTON_NONE;

  while ( result_button == LCD_KEYPAD_BUTTON_NONE 
       || result_button == LCD_KEYPAD_BUTTON_INDETERMINATE) {
    result_button = lcd_keypad_check_buttons ();
    _delay_us (poll_interval_us);
  }
  
  lcd_keypad_button_t no_button = result_button;

  // Note that overlapping button presses can't be handled using the
  // voltage-divider-and-ADC approach.  Furthermore
  while ( no_button != LCD_KEYPAD_BUTTON_NONE ) {
    no_button = lcd_keypad_check_buttons ();
    _delay_us (poll_interval_us);
  }

  return result_button; 
}

lcd_keypad_button_t
lcd_keypad_set_value (const char *name, double *value)
{
  name = name; value = value; assert (0);   // Not implemented
}

void
lcd_keypad_set_values (const char **name, double *value)
{
  name = name ; value = value; assert (0);
}
