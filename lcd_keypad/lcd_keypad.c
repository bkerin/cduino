// Implementation of the interface described in lcd_keypad.h.

#include <assert.h>
// FIXME: here only cause assert.h wrongly needs, remove when that bug is
// fixed (which it is in most recent upstream AVR libc
#include <stdlib.h>   

#include "adc.h"
#include "lcd.h"
#include "lcd_keypad.h"

void
lcd_keypad_init (void)
{
  lcd_init ();
  adc_init (ADC_REFERENCE_AVCC);
}

void
lcd_keypad_set_parameter (const char *name, double *value)
{
  lcd_home ();

  assert (0);
}
