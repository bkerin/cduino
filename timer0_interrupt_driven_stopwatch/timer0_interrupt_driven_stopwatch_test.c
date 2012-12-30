// Exercise the interface described in timer0_interrupt_driven_stopwatch.h.
//
// This program should double-blink the onboard LED on the Arduino PB5 pin
// three times (note that the normal Arduino boot sequence might blink it
// a time or two itself), with approximately 3 seconds between each blink,
// then double-blink one final time, then do nothing.

#include <assert.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
// FIXME: we shouldn't need to include this once avrlibc has a correct
// assert.h (we only use it for assert as of this writing).
#include <stdlib.h>
//#include "term_io.h"   // For debugging
#include "timer0_interrupt_driven_stopwatch.h"

static void
doubleblink_pb5 (void)
{
  double const sbtms = 200.0;   // Single blink time, in milliseconds

  PORTB |= _BV (PORTB5);
  _delay_ms (sbtms);
  PORTB &= ~(_BV (PORTB5));
  _delay_ms (sbtms);
  
  PORTB |= _BV (PORTB5);
  _delay_ms (sbtms);
  PORTB &= ~(_BV (PORTB5));
  _delay_ms (sbtms);
}

int
main (void)
{
  // term_io_init ();   // For debugging

  // Set up pin PB5 for output so we can blink the LED onboard the Arduino.
  // We don't use the dio interface here to avoid an unnecessary dependency.
  DDRB |= _BV (DDB5);
  loop_until_bit_is_set (DDRB, DDB5);
  PORTB &= ~(_BV (PORTB5));

  timer0_interrupt_driven_stopwatch_init ();

  const uint64_t tbbus = 3 * 1000000;   // Time between blinks, in us.

  uint8_t doubleblinks = 0;
  
  // Test timer monotonicity: time should always increase.
  uint16_t mtc = UINT16_MAX - 1;   // Monotonicity test count, these are cheap
  uint16_t ii;
  uint64_t old_ticks = 0;
  for ( ii = 0 ; ii < mtc ; ii++ ) {
    uint64_t new_ticks = timer0_interrupt_driven_stopwatch_ticks ();
    assert (new_ticks >= old_ticks);
    old_ticks = new_ticks;
  }

  // Test timer monotonicity with some small out-of-phase delays thrown in.
  mtc = 1042;   // These are more expensive, so we do fewer of them.
  old_ticks = 0;
  for ( ii = 0 ; ii < mtc / 1000 ; ii++ ) {
    uint64_t new_ticks = timer0_interrupt_driven_stopwatch_ticks ();
    assert (new_ticks >= old_ticks);
    old_ticks = new_ticks;
    _delay_us (ii % 242);  // Because its not 256, and ends in 42 :)
  }

  // The first in our series of doubleblinks :)
  doubleblink_pb5 ();
  doubleblinks++;
  
  // This should reset the timer to zero, we can sort of tell if it always
  // has this effect by noting if the three doubleblinks are evenly spaced.
  // Not much to go wrong here hopefully (FIXME: except possible for not
  // resetting the prescaler, ug).
  timer0_interrupt_driven_stopwatch_reset ();

  int no_reset_yet = 1;   // Flag true iff we haven't tested reset yet.

  for ( ; ; ) {

    uint64_t eus
      = timer0_interrupt_driven_stopwatch_microseconds ();   // Elapsed us

    // Verify that the ticks() method comes in with about the same reading
    // as the microseconds() method when the conversion factor is used.
    // Note that the tick_slop value used here covers the worst case I get
    // by 30 extra ticks, but doesn't constitute any sort of interface
    // gauranteeabout the maximum delay required for the function call
    // overhead.
    uint64_t eticks
      = timer0_interrupt_driven_stopwatch_ticks ();   // Elapsed ticks
    const uint64_t uspt
      = TIMER0_INTERRUPT_DRIVEN_STOPWATCH_MICROSECONDS_PER_TIMER_TICK;
    const uint64_t tick_slop = 60;
    assert (eticks - eus / uspt < tick_slop);

    if ( eus >= tbbus ) {
                
      if ( doubleblinks == 1 ) {
        doubleblink_pb5 ();
        doubleblinks++;
      }

      // Test that the reset() method takes us back to zero: it should now
      // be tbbus microseconds before we blink again.  See not above previous
      // reset() method call.
      else if ( doubleblinks == 2 && no_reset_yet ) {
        timer0_interrupt_driven_stopwatch_reset ();
        no_reset_yet = 0;
      }

      // Test the shutdown() method: after this, we should never blink again.
      else if ( doubleblinks == 2 && (! no_reset_yet) ) {
        timer0_interrupt_driven_stopwatch_shutdown ();
        assert (timer0_interrupt_driven_stopwatch_ticks () == 0);
        doubleblink_pb5 ();
        doubleblinks++;
      }
    }
  }
}
