// Test/demo for the timer1_stopwatch.h interface.
//
// This program first performs a number of tests with no visible output
// (except for term_io.h output, if that is enabled).  If all these pass,
// it gets around to triple-blinking the onboard LED on the Arduino PB5 pin.
// It then enters an infinite loop where it emits a single quick blink
// on the PB5 LED every 10 seconds.  If things go wrong, take a look at
// TIMER1_STOPWATCH_DEBUG in the Makefile for this module.

#include <assert.h>
#include <math.h>
#include <util/delay.h>

#include "timer1_stopwatch.h"

// FIXXME: this modules uses a different scheme for its test output than
// for example one_wire_master and many others.  I don't think there's a
// good reason for the difference.

// See the Makefile for this module for a convenient way to set all the
// compiler and linker flags required for debug logging to work.
#ifdef TIMER1_STOPWATCH_DEBUG
#  include "term_io.h"
#  ifndef __GNUC__
#    error GNU C is required by a nearby comma-swallowing macro
#  endif
#  define DEBUG_LOG(format, ...) printf_P (PSTR (format), ## __VA_ARGS__)
#else
#  define DEBUG_LOG(...)
#endif

#if TIMER1_STOPWATCH_PRESCALER_DIVIDER < 64
#  error This test program has been been tried only with a sufficiently large \
         prescaler divider value.  Some portions of it definitely will not    \
         pass with smaller divider settings, due to overhead in the tests     \
         themselves.
#endif

// Emit a single very quick blink on PB5
#define QUICK_PB5_BLINK() CHKP_USING (DDRB, DDB5, PORTB, PORTB5, 42.42, 1)

int
main (void)
{

#ifdef TIMER1_STOPWATCH_DEBUG
  term_io_init ();   // For debugging
#endif

  DEBUG_LOG ("\n");
  DEBUG_LOG ("CPU Frequency: %lu\n", F_CPU);
  DEBUG_LOG ("\n");

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
  DEBUG_LOG ("Monotonicity test passed\n");

  // Verify that the overflow flag works as expected.
  old_ticks = 0;
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
      BASSERT (TIFR1 & _BV (TOV1));
      assert (TIFR1 & _BV (TOV1));
      break;
    }
    old_ticks = new_ticks;
  }
  DEBUG_LOG ("Overflow flag test passed\n");

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
  // This is probably not true for the smallest prescaler settings:
  assert (tcnt1_reading1 == 0);
  DEBUG_LOG ("tcnt1_reading2: %u\n", tcnt1_reading2);
  // This is certainly not true for the smallest prescaler settings:
  assert (tcnt1_reading2 == 0);
  DEBUG_LOG ("tcnt1_reading3: %u\n", tcnt1_reading3);
  // This is certainly not true for the smallest prescaler settings:
  assert (tcnt1_reading3 == 0);
  DEBUG_LOG ("tcnt1_reading4: %u\n", tcnt1_reading4);
  assert (tcnt1_reading4 >= 2);
  // This last one should be true for the larger prescaler setting (certainly
  // for the default 64), but certainly not for the very small ones:
  assert (tcnt1_reading4 < 3);
  DEBUG_LOG ("Reset/reading latency tests passed\n");

  // Test that the timer can accurately measure the time taken by busy
  // waits.  It better be able to, since they ultimately share a clock.
  // Note that the accuracy of the timer reading for these tests is
  // highly dependent on the prescaler value.  When the requested delay
  // is precise multiple of the temporal resolution resulting from the
  // prescaler setting, the results tend to look perfect, otherwise they
  // can be somewhat less good.  The result should always be within one
  // TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK, however, at least for
  // the non-tiny presaler settings.
  double const delay1 = 2.0, delay2 = 42.0, delay3 = 4244.0, delay4 = 42042.42;
  TIMER1_STOPWATCH_RESET ();
  _delay_us (delay1);
  double d1m = TIMER1_STOPWATCH_MICROSECONDS ();   // Delay 1, Measured
  TIMER1_STOPWATCH_RESET ();
  _delay_us (delay2);
  double d2m = TIMER1_STOPWATCH_MICROSECONDS ();   // Delay 2, Measured
  TIMER1_STOPWATCH_RESET ();
  _delay_us (delay3);
  double d3m = TIMER1_STOPWATCH_MICROSECONDS ();   // Delay 3, Measured
  TIMER1_STOPWATCH_RESET ();
  _delay_us (delay4);
  double d4m = TIMER1_STOPWATCH_MICROSECONDS ();   // Delay 4, Measured
  DEBUG_LOG ("d1m: %f, d2m: %f, d3m: %f, d4m: %f\n", d1m, d2m, d3m, d4m);
  assert (fabs (d1m - delay1) <= TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK);
  assert (fabs (d2m - delay2) <= TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK);
  assert (fabs (d3m - delay3) <= TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK);
  assert (fabs (d4m - delay4) <= TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK);
  DEBUG_LOG ("Timer accuracy tests passed\n");

  // All automatic tests passed.
  DEBUG_LOG ("\n");
  DEBUG_LOG ("All automatic tests passed\n");

  CHKP ();

  // Now we're going to emit a single quick blink approximately every 10
  // seconds, forever.  Note that due to read and reset overhead, we would
  // drift continually even with a perfect clock source.
  DEBUG_LOG ("\n");
  DEBUG_LOG ("Will now blink once every ~10 s forever\n");
  uint16_t const uspsr       = 42042;      // us Per Stopwatch Reset
  uint32_t const ten_s_in_us = 10000000;   // 10 seconds in microseconds
  uint32_t       etslb_us    = 0;          // Elapsed Time Since Last Blink
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
