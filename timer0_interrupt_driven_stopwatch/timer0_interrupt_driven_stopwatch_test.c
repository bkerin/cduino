// Exercise the interface described in timer0_interrupt_driven_stopwatch.h.
//
// This program should double-blink the onboard LED on the Arduino PB5 pin
// three times (note that the normal Arduino boot sequence might blink it
// a time or two itself), with approximately 10 seconds between each
// blink, then double-blink one final time, then do nothing.

#include <assert.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
// FIXME: Not needed except for buggy assert.h header, remove once its fixed.
//#include <stdlib.h>   
#include <util/delay.h>

#include "term_io.h"
#include "timer0_interrupt_driven_stopwatch.h"

static void
doubleblink_pb5 (void)
{
  // FIXME: changing this to 100 breaks things at the moment.
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

static void
rpdb (void)
{
  double const bt = 50.0;

  int ii;
  for ( ii = 0 ; ii < 10 ; ii++ ) {
    PORTB |= _BV (PORTB5);
    _delay_ms (bt);
    PORTB &= ~(_BV (PORTB5));
    _delay_ms (bt);
  }
}

int
main (void)
{
  term_io_init ();

  // Set up pin PB5 for output so we can blink the LED onboard the Arduino.
  // We don't use the dio interface here to avoid an unnecessary dependency.
  DDRB |= _BV (DDB5);
  loop_until_bit_is_set (DDRB, DDB5);
  PORTB &= ~(_BV (PORTB5));

  timer0_interrupt_driven_stopwatch_init ();

  // FIXME: change back to 10 seconds  FIXME: chance blinks to doubleblinks
  const uint64_t tbbus = 2 * 1000000;   // Time between blinks, in us.

  uint8_t doubleblinks = 0;

  // Doubleblink once at startup.
  doubleblink_pb5 ();
  doubleblinks++;

  hit_interrupt = 0;
  for ( ; ; ) {

    hit_interrupt = 0;
    uint64_t oocount = timer0_overflow_count;
    hit_ofc = 0;
    uint64_t eus
      = timer0_interrupt_driven_stopwatch_microseconds ();   // Elapsed us
    uint64_t nocount = timer0_overflow_count;
    int8_t we_hit_ofc = 0;   // For saving value that we check in interrupt
    if ( hit_ofc ) {
        we_hit_ofc = 1;
    }

    // Verify that the ticks() method comes in with about the same reading
    // as the microseconds() method when the conversion factor is used.
    // Note that the tick_slop value used here to account for the different
    // time of the calls is pretty conservative (the real difference is
    // about 17 ticks worth of time for me), but doesn't represent any sort
    // of interface gaurantee about what the compiler might do between calls.
    // FIXME: reinsert this when the timer works correctly in general

    hit_ofc = 0;
    int8_t we_hit_ofc_twice = 0;
    uint64_t eticks
      = timer0_interrupt_driven_stopwatch_ticks ();   // Elapsed ticks
    if ( hit_ofc ) {
      we_hit_ofc_twice = 1;
    }
    const uint64_t uspt
      = TIMER0_INTERRUPT_DRIVEN_STOPWATCH_MICROSECONDS_PER_TIMER_TICK;
    const uint64_t tick_slop = 10000;
    assert (eticks - eus / uspt < tick_slop);
    uint8_t hiv = hit_interrupt; // Store so we can see what it was
    if ( eticks - eus / uspt >= tick_slop ) { 
      printf ("oocount: %ld\n", (long int) oocount);
      printf ("nocount: %ld\n", (long int) nocount);
      printf ("eticks %% 256: %ld\n", (long int) eticks % 256);
      printf ("eus: %ld\n", (long int) eus);
      printf ("uspt: %ld\n", (long int) uspt);
      printf ("tticks: %ld\n", (long int) eus / (long int) uspt);
      printf ("eticks: %ld\neus / uspt: %ld\neticks - eus / uspt: %ld\n",
          (long int) eticks,
          (long int) eus / (long int) uspt,
          (long int) eticks - (long int) eus / (long int) uspt);
      if ( eus % uspt != 0 ) {
        printf ("uneven divide\n");
      }
      if ( hiv >= 1 ) {
        printf ("hiv (hit_interrupt): %d\n", (int) hiv);
      }
      if ( we_hit_ofc == 1 ) {
        printf ("we_hit_ofc\n");
      }
      if ( we_hit_ofc_twice == 1 ) {
        printf ("we_hit_ofc_twice\n");
      }
      printf ("\n");
    }
    hit_interrupt = 0;

    if ( eus >= tbbus ) {
                
      if ( doubleblinks == 1 ) {
        doubleblink_pb5 ();
      }

      // Test the reset() method: it should now be tbbus microseconds before
      // we blink again.
      else if ( doubleblinks == 2 ) {
        timer0_interrupt_driven_stopwatch_reset ();
      }

      // Test the shutdown() method: after this, we should never blink again.
      else if ( doubleblinks == 3 ) {
        timer0_interrupt_driven_stopwatch_shutdown ();
        assert (timer0_interrupt_driven_stopwatch_ticks () == 0);
        doubleblink_pb5 ();
      }
      
      doubleblinks++;
    }
  }
}
