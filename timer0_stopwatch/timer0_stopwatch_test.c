// Test/demo for the timer0_stopwatch.h interface.
//
// This program first performs a number of internal tests with no visible
// output.  If all these pass, it gets around to tripple-blinking the onboard
// LED on the Arduino PB5 pin three times (note that the normal Arduino
// boot sequence might blink it a time or two itself), with approximately 3
// seconds between each tripple-blink, then does nothing.  If things go wrong,
// take a look at TIMER0_STOPWATCH_DEBUG in the Makefile for this module.


#include <assert.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <limits.h>
#include <util/delay.h>

#include "timer0_stopwatch.h"

// FIXXME: this modules uses a different scheme for its test output than
// for example one_wire_master and many others.  I don't think there's a
// good reason for the difference.

// See the Makefile for this module for a convenient way to set all the
// compiler and linker flags required for debug logging to work.
#ifdef TIMER0_STOPWATCH_DEBUG
#  include "term_io.h"
#  ifndef __GNUC__
#    error GNU C is required by a nearby comma-swallowing macro
#  endif
#  define DEBUG_LOG(format, ...) printf_P (PSTR (format), ## __VA_ARGS__)
#else
#  define DEBUG_LOG(...)
#endif

int
main (void)
{

#ifdef TIMER0_STOPWATCH_DEBUG
  term_io_init ();   // For debugging
#endif

  DEBUG_LOG ("\n");

  DEBUG_LOG ("CPU Frequency: %lu\n", F_CPU);

  // Set up pin PB5 for output so we can blink the LED onboard the Arduino.
  // We don't use the dio interface here to avoid an unnecessary dependency.
  DDRB |= _BV (DDB5);
  loop_until_bit_is_set (DDRB, DDB5);
  PORTB &= ~(_BV (PORTB5));

  timer0_stopwatch_init ();

  // Time between tripple-blinks, in us.
  const uint32_t tbtbus = 3 * 1000000;

  uint8_t trippleblinks = 0;

  // Test timer monotonicity: time should always increase.
  uint16_t mtc = UINT16_MAX - 1;   // These tests are fast, we will do lots.
  uint16_t ii;
  uint32_t old_ticks = 0;
  for ( ii = 0 ; ii < mtc ; ii++ ) {
    uint32_t new_ticks = timer0_stopwatch_ticks ();
    assert (new_ticks >= old_ticks);
    old_ticks = new_ticks;
  }

  // See other calls where we make some effort to verify that this function
  // actually resets the stopwatch to zero (though there isn't much to go
  // wrong here).
  timer0_stopwatch_reset ();

  // Test that the timer is monotonic and always counts at least as fast as
  // _delay_us() using some small out-of-phase delays thrown in.
  uint8_t max_delay_us = 242;   // Because it's not 256, and ends in 42 :)
  mtc = 1042;   // These tests are not so fast, we'll do fewer.
  old_ticks = 0;
  double delay_us = 0.0;
  // FIXME: this looks wrong, won't it always quit at ii = 2?
  for ( ii = 0 ; ii < mtc / 1000 ; ii++ ) {
    uint32_t new_ticks = timer0_stopwatch_ticks ();
    const int uspt = TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK;
    assert (new_ticks >= old_ticks + delay_us / uspt);
    old_ticks = new_ticks;
    delay_us = ii % (max_delay_us + 1);
    // FIXME: THIS IS BROKEN: _delay_us requires a compile-time constant
    // argument, we we need a delay wrapper that works in constants or
    // something
    _delay_us (delay_us);
  }

  // Now that we think we have a working counter, lets measure
  // our read overhead.  First we'll measure the overhead for the
  // TIMER0_STOPWATCH_TICKS() macro.
  const uint16_t omrc = 5042;  // Overhead Measurement Read Count
  // The overhead_ticks is the number of ticks used in a loop that does
  // nothing but read.  Not declaring this volatile sometimes result in
  // slightly faster code, but it doesn't necessarily, since what the
  // optimizer does depends on how the value is referenced elsewhere.  Since
  // we want consistent worst-case behavior, we declare this value volatile.
  volatile uint32_t overhead_ticks;
  timer0_stopwatch_reset ();
  for ( ii = 0 ; ii < omrc ; ii++ ) {
    TIMER0_STOPWATCH_TICKS (overhead_ticks);
  }
  int mot = TIMER0_STOPWATCH_TICKS_MACRO_MAX_OVERHEAD_TICKS;
  assert ((double) overhead_ticks / omrc <=  mot);
  DEBUG_LOG (
      "TIMER0_STOPWATCH_TICKS() macro approx. overhead ticks per read: "
      "%f\n",
      (double) overhead_ticks / omrc );

  // Now we'll measure the overhead of the timer0_stopwatch_ticks() function.
  timer0_stopwatch_reset ();
  for ( ii = 0 ; ii < omrc ; ii++ ) {
    overhead_ticks = timer0_stopwatch_ticks ();
  }
  mot = TIMER0_STOPWATCH_TICKS_FUNCTION_MAX_OVERHEAD_TICKS;
  assert ((double) overhead_ticks / omrc <= mot);
  DEBUG_LOG (
      "timer0_stopwatch_ticks() function approx. overhead ticks per read: "
      "%f\n",
      (double) overhead_ticks / omrc );

  // Now we'll measure the overhead of the timer0_stopwatch_microseconds()
  // function.
  volatile uint32_t overhead_microseconds;
  timer0_stopwatch_reset ();
  for ( ii = 0 ; ii < omrc ; ii++ ) {
    overhead_microseconds = timer0_stopwatch_microseconds ();
  }
  int mous = TIMER0_STOPWATCH_MICROSECONDS_FUNCTION_MAX_READ_OVERHEAD_US;
  assert ((double) overhead_microseconds / omrc <= mous);
  DEBUG_LOG (
      "timer0_stopwatch_microseconds() function approx. overhead us per read: "
      "%f\n",
      (double) overhead_microseconds / omrc );

  // Test the latency performance of the TIMER0_STOPWATCH_RESET_TCNT0()
  // and TIMER0_STOPWATCH_TCNT0() macros.
  TIMER0_STOPWATCH_RESET_TCNT0 ();
  uint8_t tcnt0_reading1 = TIMER0_STOPWATCH_TCNT0 ();
  uint8_t tcnt0_reading2 = TIMER0_STOPWATCH_TCNT0 ();
  _delay_us (1.0);
  uint8_t tcnt0_reading3 = TIMER0_STOPWATCH_TCNT0 ();
  _delay_us (2.0 * TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK);
  uint8_t tcnt0_reading4 = TIMER0_STOPWATCH_TCNT0 ();
  DEBUG_LOG ("tcnt0_reading1: %u\n", tcnt0_reading1);
  assert (tcnt0_reading1 == 0);
  DEBUG_LOG ("tcnt0_reading2: %u\n", tcnt0_reading2);
  assert (tcnt0_reading2 == 0);
  DEBUG_LOG ("tcnt0_reading3: %u\n", tcnt0_reading3);
  assert (tcnt0_reading3 == 0);
  DEBUG_LOG ("tcnt0_reading4: %u\n", tcnt0_reading4);
  assert (tcnt0_reading4 >= 2);
  assert (tcnt0_reading4 < 3);

  // The first in our series of trippleblinks :)
  CHKP ();
  trippleblinks++;

  // This should reset the timer to zero, we can sort of tell if it always
  // has this effect by noting if the three trippleblinks are evenly spaced.
  // Not much to go wrong here hopefully.
  timer0_stopwatch_reset ();

  int no_reset_yet = 1;   // Flag true iff we haven't tested reset yet.

  uint64_t ous = 0;  // Old elapsed microseconds reading (on last iteration)

  for ( ; ; ) {

    uint64_t eus
      = timer0_stopwatch_microseconds ();   // Elapsed us

    // Check for timer overflow
    ous = eus;
    if ( ous > eus ) {
      DEBUG_LOG ("OVERFLOW DETECTED\n");
      assert (ous <= ULONG_MAX);
      DEBUG_LOG (
          "Overflow detected after %lu microseconds\n",
          (long unsigned) ous);
    }

    // Verify that the ticks() method comes in with about the same reading
    // as the microseconds() method when the conversion factor is used.
    // Note that the tick_slop value used here covers the worst case I get
    // by 30 extra ticks, but doesn't constitute any sort of interface
    // gauranteeabout the maximum delay required for the function call
    // overhead.
    uint64_t eticks
      = timer0_stopwatch_ticks ();   // Elapsed ticks
    const uint64_t uspt
      = TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK;
    const uint64_t tick_slop = 60;
    assert (eticks - eus / uspt < tick_slop);

    if ( eus >= tbtbus ) {

      if ( trippleblinks == 1 ) {
        CHKP ();
        trippleblinks++;
      }

      // Test that the reset() method takes us back to zero: it should now
      // be tbtbus microseconds before we blink again.  See not above previous
      // reset() method call.
      else if ( trippleblinks == 2 && no_reset_yet ) {
        timer0_stopwatch_reset ();
        no_reset_yet = 0;
      }

      // Test the shutdown() method: after this, we should never blink again.
      else if ( trippleblinks == 2 && (! no_reset_yet) ) {
        timer0_stopwatch_shutdown ();
        assert (timer0_stopwatch_ticks () == 0);
        uint32_t macro_read_ticks;
        TIMER0_STOPWATCH_TICKS (macro_read_ticks);
        assert (macro_read_ticks == 0);
        assert (timer0_stopwatch_microseconds () == 0);
        CHKP ();
        trippleblinks++;
        DEBUG_LOG ("All tests succeeded.\n");
      }
    }
  }
}
