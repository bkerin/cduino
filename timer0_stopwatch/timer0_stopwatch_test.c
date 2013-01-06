// Exercise the interface described in timer0_stopwatch.h.
//
// This program first performs a number of internal tests with no visible
// output.  If all these pass, it get around to tripple-blinking the onboard
// LED on the Arduino PB5 pin three times (note that the normal Arduino boot
// sequence might blink it a time or two itself), with approximately 3 seconds
// FIXME: SOOO: three trippleblinks, or four?  between each tripple-blinks,
// then tripple-blink one final time, then do nothing.

#include <assert.h>
#include <util/atomic.h>   // FIXME: for debugging/profiling
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <limits.h>
#include <util/delay.h>
// FIXME: we shouldn't need to include this once avrlibc has a correct
// assert.h (we only use it for assert as of this writing).
#include <stdlib.h>
#include "term_io.h"   // For debugging
#include "timer0_stopwatch.h"

// Given an overflow counter size in bytes, return a safe and
// not-too-time-comsuming number of reads (plus test housekeeping )we can
// perform without overflowing the timer (which would result in a monotonicy
// test failure).
static uint16_t
monotonicity_test_count (uint8_t overflow_counter_size)
{
  uint16_t result;

  switch ( overflow_counter_size ) {
    case 1:
      assert (0); // not determined yet
      break;
    case 2:
      result = 4042;
      break;
    case 4:
      result = UINT16_MAX - 1;
      break;
    case 8:
      result = UINT16_MAX - 1;
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }

  return result;
}

// Given an overflow counter size in bytes and a mean delay per test in
// microseconds, return a safe and not-too-time-comsuming number of reads
// (plus test housekeeping )we can perform without overflowing the timer
// (which would result in a monotonicy test failure).
static uint16_t
monotonicity_with_delays_test_count (
    uint8_t overflow_counter_size,
    uint8_t mean_delay_per_read_us)
{
  uint16_t result;

  // This is what we assume about the per-read delay from our calling context.
  assert (mean_delay_per_read_us <= 130);
  
  switch ( overflow_counter_size ) {
    case 1:
      assert (0); // not determined yet
      break;
    case 2:
      result = 1042;
      break;
    case 4:
      result = 1042;
      break;
    case 8:
      result = 1042; 
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }

  return result;
}

int
main (void)
{
  term_io_init ();   // For debugging

  if ( sizeof (timer0_stopwatch_rt) < 4 ) {
    printf (
        "Sorry, this test driver isn't set up to test types smaller than"
        "32 bits wide.\n");
    assert (0);   // Shouldn't be here.
  }

  // Set up pin PB5 for output so we can blink the LED onboard the Arduino.
  // We don't use the dio interface here to avoid an unnecessary dependency.
  DDRB |= _BV (DDB5);
  loop_until_bit_is_set (DDRB, DDB5);
  PORTB &= ~(_BV (PORTB5));

  timer0_stopwatch_init ();

  // Time between tripple-blinks, in us.
  const timer0_stopwatch_rt tbtbus = 3 * 1000000;   

  uint8_t trippleblinks = 0;

  // Test timer monotonicity: time should always increase.
  uint16_t mtc = monotonicity_test_count (sizeof (timer0_stopwatch_oct));
  uint16_t ii;
  timer0_stopwatch_rt old_ticks = 0;
  for ( ii = 0 ; ii < mtc ; ii++ ) {
    timer0_stopwatch_rt new_ticks = timer0_stopwatch_ticks ();
    assert (new_ticks >= old_ticks);
    old_ticks = new_ticks;
  }

  // See other calls where we make some effort to verify that this function
  // actually resets the stopwatch to zero (though there isn't much to go
  // wrong here).
  timer0_stopwatch_reset ();

  // Test that the timer is monotonic and always counts at least as fast as
  // _delay_us() using some small out-of-phase delays thrown in.
  uint8_t max_delay_us = 242;   // Because its not 256, and ends in 42 :)
  mtc = monotonicity_with_delays_test_count (
      sizeof (timer0_stopwatch_oct),
      max_delay_us / 2);
  old_ticks = 0;
  double delay_us = 0.0;
  for ( ii = 0 ; ii < mtc / 1000 ; ii++ ) {
    timero_stopwatch_rt new_ticks = timer0_stopwatch_ticks ();
    const int uspt = TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK ;
    assert (new_ticks >= old_ticks + delay_us / uspt);
    old_ticks = new_ticks;
    delay_us = ii % (max_delay_us + 1);
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
  volatile timer0_stopwatch_rt overhead_ticks;   
  timer0_stopwatch_reset ();
  for ( ii = 0 ; ii < omrc ; ii++ ) {
    TIMER0_STOPWATCH_TICKS (overhead_ticks);
  }
  int mot = TIMER0_STOPWATCH_TICKS_MACRO_MAX_OVERHEAD_TICKS;
  assert (overhead_ticks / omrc <= mot);
  printf (
      "TIMER0_STOPWATCH_TICKS() macro approx. overhead ticks per read: "
      "%f\n",
      (double) overhead_ticks / omrc );

  // Now we'll measure the overhead of the timer0_stopwatch_ticks() function. 
  timer0_stopwatch_reset ();
  for ( ii = 0 ; ii < omrc ; ii++ ) {
    overhead_ticks = timer0_stopwatch_ticks ();
  }
  mot = TIMER0_STOPWATCH_TICKS_FUNCTION_MAX_OVERHEAD_TICKS;
  assert (overhead_ticks / omrc <= mot);
  printf (
      "timer0_stopwatch_ticks() function approx. overhead ticks per read: "
      "%f\n",
      (double) overhead_ticks / omrc );
  
  // Now we'll measure the overhead of the timer0_stopwatch_microseconds()
  // function.
  volatile timer0_stopwatch_rt overhead_microseconds;
  timer0_stopwatch_reset ();
  for ( ii = 0 ; ii < omrc ; ii++ ) {
    overhead_microseconds = timer0_stopwatch_microseconds ();
  }
  int mous = TIMER0_STOPWATCH_MICROSECONDS_FUNCTION_MAX_READ_OVERHEAD_US;
  assert (overhead_microseconds / omrc <= mous);
  printf (
      "timer0_stopwatch_microseconds() function approx. overhead us per read: "
      "%f\n",
      (double) overhead_microseconds / omrc );

  // The first in our series of trippleblinks :)
  CHKP;
  trippleblinks++;

  // This should reset the timer to zero, we can sort of tell if it always
  // has this effect by noting if the three trippleblinks are evenly spaced.
  // Not much to go wrong here hopefully (FIXME: except possible for not
  // resetting the prescaler, ug).
  timer0_stopwatch_reset ();

  int no_reset_yet = 1;   // Flag true iff we haven't tested reset yet.
    
  printf ("FIXME: cp-2\n");

  uint64_t ous = 0;  // Old elapsed microseconds reading (on last iteration)

  for ( ; ; ) {
    
    uint64_t eus
      = timer0_stopwatch_microseconds ();   // Elapsed us

    // Check for timer overflow
    ous = eus;
    if ( ous > eus ) {
      printf ("OVERFLOW DETECTED\n");
      assert (ous <= ULONG_MAX);
      printf (
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

    //printf ("FIXME: cp0\n");

    if ( eus >= tbtbus ) {
                
      if ( trippleblinks == 1 ) {
        CHKP;
        printf ("FIXME: cp1\n");
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
        CHKP;
        trippleblinks++;
      }
    }
  }
}
