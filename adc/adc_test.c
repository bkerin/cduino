// Test/demo for the adc.h interface.

// Assumptions:
//
// FIXME: ensure that these comments end up agreeing with the code as to
// which pin number for led and which for A2D
// 	- 10 kohm (or so) potentiometer connected between 5V supply and ground,
// 	  with potentionmeter tap connected to pin A1 (aka PC1).
//
//	- LED connected from pin A0 (aka PC0) to ground.  With a current
//	  limiting resistor in series if you like to be prim and proper :).
//	  This is only required in order to test that we can use the
//	  individual ADC pins independently for different purposes.
//
//	  FIXME: audit item: in shield modules at least, we refer to
//	  pins by Arduino names first, with "(aka major-datasheet-name
//	  aka other-datasheet-name) tacked on as appropriate.  At least
//	  the major-datasheet-name is probably always appropriate.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>   // FIXME: remove once assert.h header is fixed
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "term_io.h"
#include "adc.h"

// This function is used to help verify that ADC pins can still be used
// when other ADC pins are being used as digital output.  Require pin A1
// (aka PC1) to be configured as an output.  Toggle it.
static void
toggle_pc1 (void)
{
  if ( PORTC & _BV (PORTC1) ) {
    PORTC &= ~(_BV (PORTC1));
    // FIXXME: could be a no-op, once we have avr libc that has that
    loop_until_bit_is_clear (PORTC, PORTC1);
  }
  else {
    PORTC |= _BV (PORTC1);
    // FIXXME: could be a no-op, once we have avr libc that has that
    loop_until_bit_is_set (PORTC, PORTC1);
  }
}

int
main (void)
{
  //PGM_P pot_fmtstr = "Potentiometer tap voltage: %f (%d raw)\r\n";
  //char * pot_fmtstr = "Potentiometer tap voltage: %f (%d raw)\r\n";

  term_io_init ();   // Set up terminal communications.

  // FIXME: audit item: I like to put const after type as Dan Saks advises
  uint8_t const aip = 0;   // Analog Input Pin (from 0 for ADC0 to 5 for ADC5)

  adc_init (ADC_REFERENCE_AVCC);
  adc_pin_init (aip);

  // This register bit is hardwired to match the chosen aip value.
  if ( ! (DIDR0 & _BV (ADC0D)) ) {
    printf (
        "failure: Digital input disable bit ADC1D of register DIDR0 not "
        "set\n" );
    assert (0);
  }
  
  // Configure pin A1 (aka PC1) as an output, starting out low.
  PORTC &= ~(_BV (PORTC1));
  //FIXXME: could be a no-op, once we have avr libc that has that
  loop_until_bit_is_clear (PORTC, PORTC1);
  DDRC |= _BV (DDC1);

  while ( 1 )
  {
    float const supply_voltage = 5.0;

    uint16_t raw = adc_read_raw (aip);
    float tap_voltage = adc_read_voltage (aip, supply_voltage);

    printf_P (
        PSTR ("Potentiometer tap voltage: %f (%d raw)\r\n"),
        tap_voltage,
        raw );

    toggle_pc1 ();

    float mspr = 500.0;   // Milliseconds Per Reading (and per LED toggle)
    _delay_ms (mspr);
  }
  return 0;
}

