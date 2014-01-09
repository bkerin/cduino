// Test/demo for the adc.h interface.

// Assumptions:
//
//      - 10 kohm (or so) potentiometer connected between 5V supply
//        and ground, with potentionmeter tap connected to pin A0 (aka PC0
//        aka ADC0).  Alternately, a simple voltage divider can be used
//        (but then of course the ADC behavior can only be verified at a
//        single point).
//
//      - LED connected from pin A1 (aka PC1) to ground.  With a current
//        limiting resistor in series if you like to be prim and proper :).
//        This is only required in order to test that we can use the
//        individual ADC pins independently for different purposes.
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

#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
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
  // This isn't what we're testing exactly, but we need to know if its
  // working or not to interpret other results.
  term_io_init ();
  PFP ("\n");
  PFP ("\n");
  PFP ("term_io_init() worked.\n");
  PFP ("\n");

  // FIXME: audit item: I like to put const after type as Dan Saks advises
  uint8_t const aip = 0;   // Analog Input Pin (from 0 for ADC0 to 5 for ADC5)

  adc_init (ADC_REFERENCE_AVCC);
  PFP ("Finished adc_init().\n");
  adc_pin_init (aip);
  // This register bit test is hardwired to match the chosen aip value.
  // The initialization should have done this, but we can't tell just by
  // observing that the ADC reads voltages correctly, so we check here.
  if ( ! (DIDR0 & _BV (ADC0D)) ) {
    PFP (
        "failure: Digital input disable bit ADC0D of register DIDR0 not "
        "set\n" );
    assert (0);
  }
  PFP ("Finished adc_pin_init().\n");

  PFP ("\n");
  
  // Configure pin A1 (aka PC1) as an output, starting out low.
  PORTC &= ~(_BV (PORTC1));
  //FIXXME: could be a no-op, which recent avr libc have a macro for
  loop_until_bit_is_clear (PORTC, PORTC1);
  DDRC |= _BV (DDC1);

  while ( 1 )
  {
    float const supply_voltage = 5.0;

    uint16_t raw = adc_read_raw (aip);
    float tap_voltage = adc_read_voltage (aip, supply_voltage);

    PFP ("ADC input voltage: %f (%d raw)\r\n", tap_voltage, raw);

    toggle_pc1 ();

    float mspr = 500.0;   // Milliseconds Per Reading (and per LED toggle)
    _delay_ms (mspr);
  }
  return 0;
}

