// Implementation of the interface described in lcd_keypad.h.

#include <assert.h>
#include <avr/pgmspace.h>
// FIXME: here only cause assert.h wrongly needs, remove when that bug is
// fixed (which it is in most recent upstream AVR libc
#include <stdlib.h>   
#include <string.h>
#include <util/delay.h>

#include "adc.h"
#include "lcd.h"
#include "lcd_keypad.h"
#include "util.h"

void
lcd_keypad_init ()
{
  lcd_init ();
  adc_init (ADC_REFERENCE_AVCC);
  adc_pin_init (LCD_KEYPAD_ADC_PIN);
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

// NOTE: this poll interval is probably on the paranoid side.  I don't know
// of any good source of information for guidance on this, so this value
// is a combination of trial and paranoia.
static const double poll_interval_us = 100; 
 
#define BUTTON_COUNT 5
  
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
  assert (ADC_RAW_READING_STEPS == 1024);
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
  // have a definite button press at that value.
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

  while ( result_button == LCD_KEYPAD_BUTTON_NONE ||
          result_button == LCD_KEYPAD_BUTTON_INDETERMINATE ) {
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

// Update a value display displayed on the second line of the LCD.
static void
update_value_on_lcd (double value)
{
  lcd_set_cursor_position (0, 1);
  lcd_printf_P (PSTR(LCD_KEYPAD_VALUE_DISPLAY_FORMAT), value);
}

// Update the part of the display showing a value (see callers).
lcd_keypad_button_t
lcd_keypad_show_value (const char *name, double *value)
{
  // Draw the field name in top line of LCD, and current value in next line.
  lcd_clear ();
  lcd_home ();
  // Note that we truncate really long variable names as per the
  // LCD_KEYPAD_VALUE_NAME_MAX_LENGTH interface macro.
  lcd_printf_P (PSTR ("%.15s:"), name);
  update_value_on_lcd (*value);

  lcd_keypad_button_t button = lcd_keypad_wait_for_button ();

  return button;
}

// Repeatedly poll the buttons (every poll_interval_us microseconds)
// for about stw (seconds to wait) seconds, returning true only when an
// LCD_KEYPAD_BUTTON_NONE value is read, or false if that value is never read.
// If stw is negative, wait forever.
static int
timed_wait_for_button_none (double stw)
{
  double sw = 0.0;   // Seconds waited so far.

  lcd_keypad_button_t button = lcd_keypad_check_buttons ();

  while ( button != LCD_KEYPAD_BUTTON_NONE && (stw < 0 || sw < stw)) {
    const double seconds_per_us = 1000000.0; 
    // These constants are due to the way the ADC works (13 ADC clock cycles
    // per sample), the way the adc_read_raw() function is implemented
    // (assumes 125 kHz ADC clock) and the way the check_buttons method is
    // implemented (2 ADC reads per call).  The fudge factor would ideally
    // be zero since its probably somewhat wrong for non-default processor
    // speeds, small changes to the adc_read_raw implementation, etc.
    // But we don't promise anything about the exact delays in the lcd_keypad
    // interface anyway.
    const double 
      adc_cycles_per_sample            = 13.0,
      adc_frequency                    = 125000.0,
      adc_reads_per_check_buttons_call = 2.0,
      fudge_factor                     = 1.5;
    // Time per lcd_keypad_check_button() call.
    const double tpcbc
      = fudge_factor * (
          adc_reads_per_check_buttons_call * adc_cycles_per_sample
          * (1.0 / adc_frequency) + (poll_interval_us / seconds_per_us));
    sw += tpcbc;
    button = lcd_keypad_check_buttons ();
  }

  return button == LCD_KEYPAD_BUTTON_NONE;
}

lcd_keypad_button_t
lcd_keypad_set_value (const char *name, double *value, double step)
{
  lcd_keypad_button_t button = LCD_KEYPAD_BUTTON_NONE;

  // Draw the field name in top line of LCD, and current value in next line.
  lcd_clear ();
  lcd_home ();
  // Note that we truncate really long variable names as per the
  // LCD_KEYPAD_VALUE_NAME_MAX_LENGTH interface macro.
  lcd_printf_P (PSTR ("%.15s:"), name);
  update_value_on_lcd (*value);

  // Timing parameters for button hold-down repeating: Time till repeat
  // starts, repeat frequency, and screen update frequency during repeat.
  // The suf value is intended to help us cope with the fact that the LCD
  // doesn't refresh very quickly and would be unreadable much if updated
  // continually.  The better way would be to update just the changing digits,
  // but this is quite a pain in the neck and would waste code space.  NOTE:
  // we could expose rf in this function's interface to allow clients to get
  // control of fast-scale vs. slow scale ratio, but it adds complexity that
  // I don't think is really worthwhile.
  const double ttr = 1.5, rf = 10.0, suf = 2.0;

  // True iff no button has been held down long enough for repeating to start.
  int not_repeating = TRUE;
  int rssu = 0;   // Repeats since screen update (for held down buttons).

  while ( button != LCD_KEYPAD_BUTTON_RIGHT &&
          button != LCD_KEYPAD_BUTTON_LEFT &&
          button != LCD_KEYPAD_BUTTON_SELECT ) {
    while ( button == LCD_KEYPAD_BUTTON_NONE ||
            button == LCD_KEYPAD_BUTTON_INDETERMINATE ) {
      button = lcd_keypad_check_buttons ();
      _delay_us (poll_interval_us);
    }

    // Incrememt or decrement value, or wait forever until the button is
    // released if it's one of the ones that ends the selection.
    if ( button == LCD_KEYPAD_BUTTON_UP ) {
      *value += step;
    }
    else if ( button == LCD_KEYPAD_BUTTON_DOWN ) {
      *value -= step;
    }
    else if ( button == LCD_KEYPAD_BUTTON_RIGHT ||
              button == LCD_KEYPAD_BUTTON_LEFT ||
              button == LCD_KEYPAD_BUTTON_SELECT ) {
      timed_wait_for_button_none (-1.0);
      break;
    }
    else {
      assert (0);   // Shouldn't be here.
    }
    
    if ( not_repeating ) {
      update_value_on_lcd (*value);
      int released = timed_wait_for_button_none (ttr);
      if ( ! released ) {
        not_repeating = FALSE;
        update_value_on_lcd (*value);
      }
      else {
        button = LCD_KEYPAD_BUTTON_NONE;
      }
    }
    else {
      int released = timed_wait_for_button_none (1.0 / rf);
      rssu++;
      if ( rssu * 1.0 / rf >= 1.0 / suf ) {
        update_value_on_lcd (*value);
        rssu = 0;
      }
      if ( released ) {
        not_repeating = TRUE;
        button = LCD_KEYPAD_BUTTON_NONE;
        update_value_on_lcd (*value);
        rssu = 0;
      }
    }
  }
       
  // If we were a ! not_repeating state we might need one last update here.
  // A bit of paranoia to make sure we end up displaying the true *value.
  update_value_on_lcd (*value);

  return button;
}
