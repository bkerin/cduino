// Test driver for the adc.h interface.

// Assumptions:
//
// 	- 10 kohm (or so) potentiometer connected between 5V supply and ground,
// 	  with potentionmeter tap connected to pin PC0 (ADC0).

#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "term_io.h"
#include "adc.h"

int
main (void)
{
  uint16_t raw;
  PGM_P pot_fmtstr = "Potentiometer tap voltage: %f (%d raw)\r\n";

  term_io_init ();   // Set up terminal communications.

  adc_init (ADC_REFERENCE_AVCC);

  while ( 1 )
  {
    const uint8_t pin_to_read = 0;
    const float supply_voltage = 5.0;

    uint16_t raw = adc_read_raw (pin_to_read);
    float tap_voltage = adc_read_voltage (pin_to_read, supply_voltage);

    printf (pot_fmtstr, tap_voltage, raw);

    _delay_ms (500);
  }
  return 0;
}

