// Blink a led attached to pin PB5 on an off.

#include <avr/io.h>
#include <util/delay.h>

#include "util.h"

//
// Assumptions:
// 	- LED connected to PORTB (arduino boards have LED L onboard)
// 	- F_CPU is defined to be your cpu speed (preprocessor define)
//

// WARNING: this technique doesn't translate to all the other IO pins on a
// typical arduino, because the arduino bootloader uses some of them for its
// own purposes (e.g. PD0 is set up as the RX pin for serial communication,
// which precludes its use as an output).  The unconnected IO pins are
// presumably ok to use, or you can just nuke the bootloader with an
// AVRISPmkII or similar device.

static void
set_pin_pb5_for_output (uint8_t initial_value)
{
  DDRB |= _BV (DDB5);
  loop_until_bit_is_set (DDRB, DDB5);

  if ( initial_value == HIGH ) {
    PORTB |= _BV (PORTB5);
    loop_until_bit_is_set (PORTB, PORTB5);
  }
  else {
    PORTB &= ~(_BV (PORTB5));
    loop_until_bit_is_clear (PORTB, PORTB5);
  }
}

static void
set_pin_pb5 (uint8_t value)
{
  if ( value == HIGH ) {
    PORTB |= _BV (PORTB5);
    loop_until_bit_is_set (PORTB, PORTB5);
  }
  else {
    PORTB &= ~(_BV (PORTB5));
    loop_until_bit_is_clear (PORTB, PORTB5);
  }
}

int
main (void)
{
  set_pin_pb5_for_output (HIGH);

  const double blink_time_ms = 400;

  while ( 1 ) {
    _delay_ms (blink_time_ms);
    set_pin_pb5 (LOW);
    _delay_ms (blink_time_ms);
    set_pin_pb5 (HIGH);
  }
}

