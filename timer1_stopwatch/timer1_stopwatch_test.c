// Test/demo for the timer1_stopwatch.h interface.
//
// This program first performs a number of internal tests with no visible
// output.  If all these pass, it gets around to tripple-blinking the onboard
// LED on the Arduino PB5 pin three times (note that the normal Arduino
// boot sequence might blink it a time or two itself), with approximately 3
// seconds between each tripple-blink, then does nothing.  If things go wrong,
// take a look at TIMER1_STOPWATCH_DEBUG in the Makefile for this module.

#include <assert.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <limits.h>
#include <math.h>
#include <util/delay.h>

#include "timer1_stopwatch.h"

#if TIMER1_STOPWATCH_PRESCALER_DIVIDER < 64
#  error This test program has been been tried only with a sufficiently large \
         prescaler divider value.  It might work for smaller values, but then \
         again the tests themselves might be bad in that case.
#endif

// See the Makefile for this module for a convenient way to set all the
// compiler and linker flags required for debug logging to work.
#ifdef TIMER1_STOPWATCH_DEBUG
#  include "term_io.h"
#  define DEBUG_LOG(...) printf (__VA_ARGS__)
#else
#  define DEBUG_LOG(...)
#endif

// Emit a single very quick blink on PB5
// FIXME: is this long enough to be seen?
#define QUICK_PB5_BLINK() CHKP_USING (DDRB, DDB5, PORTB, PORTB5, 10.42, 1)

// AVR libc's _delay_us() requires a compile-time constant argument.  Its a
// convenient delay soure to test against, so we have this crazyness to give
// us a half-way decent form of it that can handle a variable argument.
// Sort of.  Obviously there's some overhead, and it doesn't even try to
// be closer than 10 us.
#define DELAY_APPROX_US(time)    \
  do {                           \
    double XxX_tr = time;        \
    while ( XxX_tr > 10000.0 ) { \
      _delay_us (10000.0);       \
      XxX_tr -= 10000.0;         \
    }                            \
    while ( XxX_tr > 1000.0 ) {  \
      _delay_us (1000.0);        \
      XxX_tr -= 1000.0;          \
    }                            \
    while ( XxX_tr > 100.0 ) {   \
      _delay_us (100.0);         \
      XxX_tr -= 100.0;           \
    }                            \
    while ( XxX_tr > 10.0 ) {    \
      _delay_us (10.0);          \
      XxX_tr -= 10.0;            \
    }                            \
  } while ( 0 );

int
main (void)
{

#ifdef TIMER1_STOPWATCH_DEBUG
  term_io_init ();   // For debugging
#endif

  DEBUG_LOG ("\n");

  DEBUG_LOG ("CPU Frequency: %lu\n", F_CPU);

  // Set up pin PB5 for output so we can blink the LED onboard the Arduino.
  // We don't use the dio interface here to avoid an unnecessary dependency.
  // FIXME: I think we don't need this now that we're using the blink macros
  DDRB |= _BV (DDB5);
  loop_until_bit_is_set (DDRB, DDB5);
  PORTB &= ~(_BV (PORTB5));

  timer1_stopwatch_init ();

  // Test timer monotonicity: time should always increase, as long as we
  // don't go long enough to wrap it around.
  uint16_t tc = 4242;   // Test Count.  These tests are fast, we will do lots.
  uint16_t old_ticks = 0;
  for ( uint16_t ii = 0 ; ii < tc ; ii++ ) {
    uint16_t new_ticks = TIMER1_STOPWATCH_TICKS ();
    assert (new_ticks >= old_ticks);
    old_ticks = new_ticks;
  }

  // Verify that the overflow flag works as expected.
  TIMER1_STOPWATCH_RESET ();
  for ( ; ; ) {
    // This margin allows for the possibility of overflow between the time
    // we look at the watch and the time we check the overflow flag.
    uint16_t const tick_margin = 142;
    uint16_t new_ticks = TIMER1_STOPWATCH_TICKS ();
    if ( old_ticks < new_ticks - tick_margin ) {
      assert (! (TIFR1 & _BV (TOV1)));
    }
    // If we've already gone backwards in time, the overflow flag better
    // already be set.
    if ( new_ticks < old_ticks ) {
      assert (TIFR1 & _BV (TOV1));
      break;
    }
    old_ticks = new_ticks;
  }

  // Test the latency performance of the TIMER1_STOPWATCH_RESET() and
  // TIMER1_STOPWATCH_TICKS() macros.
  TIMER1_STOPWATCH_RESET ();
  uint16_t tcnt1_reading1 = TIMER1_STOPWATCH_TICKS ();
  uint16_t tcnt1_reading2 = TIMER1_STOPWATCH_TICKS ();
  _delay_us (1.0);
  uint16_t tcnt1_reading3 = TIMER1_STOPWATCH_TICKS ();
  _delay_us (2.0 * TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK);
  uint8_t tcnt1_reading4 = TIMER1_STOPWATCH_TICKS ();
  DEBUG_LOG ("tcnt1_reading1: %u\n", tcnt1_reading1);
  assert (tcnt1_reading1 == 0);
  DEBUG_LOG ("tcnt1_reading2: %u\n", tcnt1_reading2);
  assert (tcnt1_reading2 == 0);
  DEBUG_LOG ("tcnt1_reading3: %u\n", tcnt1_reading3);
  assert (tcnt1_reading3 == 0);
  DEBUG_LOG ("tcnt1_reading4: %u\n", tcnt1_reading4);
  assert (tcnt1_reading4 >= 2);
  assert (tcnt1_reading4 < 3);

  // Test that the timer can accurately measure the time taken by busy
  // waits.  It better be alble to, since they ultimately share a clock.
  // Note that this test isn't requiring much accuracy for the larger
  // values of TIMER1_STOPWATCH_PRESCALER_DIVIDER. FIXME: investigate what
  // the mismatch might be, think about it a bit more, and test this with
  // longer delay times.
  tc = 142;
  for ( uint16_t ii = 0 ; ii < tc ; ii++ ) {
    // Test Times Increment (ms).  Note that this must be chosen with the
    // limitations of DELAY_APPROX_US() in mind.
    double const tti_us = 10.0;
    // Might as well test delay times in order.  Note that if tc is larger than
    double test_delay_us = tc * tti_us;
    TIMER1_STOPWATCH_RESET ();
    DELAY_APPROX_US (test_delay_us);
    double timer_elapsed_us = TIMER1_STOPWATCH_MICROSECONDS ();
    double tdiff = timer_elapsed_us - test_delay_us;   // Time Difference
    DEBUG_LOG ("timer_elapsed_us - test_delay_us: %f\n", tdiff);
    double const tolerable_mismatch
      = TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK;
    assert (fabs (tdiff) <= tolerable_mismatch);
  }

  // All automatic tests passed.
  CHKP ();

  // Now we're going to emit a single quick blink approximately every 10
  // seconds, forever.  note that due to read and reset overhead, we would
  // drift continually even with a perfect clock source.
  uint16_t const uspsr       = 42042;      // us Per Stopwatch Reset
  uint32_t const ten_s_in_us = 10000000;   // 10 seconds in microseconds
  uint32_t       etslb_us    = 0;         // Elapsed Time (us) Since Last Blink
  TIMER1_STOPWATCH_RESET ();
  for ( ; ; ) {
    if ( TIMER1_STOPWATCH_MICROSECONDS () >= uspsr )  {
      etslb_us += uspsr;
      TIMER1_STOPWATCH_RESET ();
    }
    if ( etslb_us > ten_s_in_us ) {
      etslb_us = 0;
      QUICK_PB5_BLINK ();
    }
  }
}
