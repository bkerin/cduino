// Implementation of the interface described in timer1_stopwatch.h.

#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/atomic.h>

#include "timer1_stopwatch.h"
#include "util.h"

// Default values of the timer/counter1 control registers (for the ATmega328P
// at least), according to the datasheet.
#define TCCR1A_DEFAULT_VALUE 0x00
#define TCCR1B_DEFAULT_VALUE 0x00

void
timer1_stopwatch_init (void)
{
  power_timer1_enable ();   // Ensure timer1 not shut down to save power

  // NOTE: these defaults correspond to the normal
  // count-up-overflow-at-the-top operation with all fancy optional timer
  // features disabled.
  TCCR1A = TCCR1A_DEFAULT_VALUE;
  TCCR1B = TCCR1B_DEFAULT_VALUE;

  // Reset the timer, in case it currently has some strange value that might
  // cause it to overflow as soon as we start it running.  This modules
  // doesn't use interrupts but just in case the user wants to use them :)
  TCNT1 = 0;

  // Ensure that the clock source for timer/counter is set to the
  // TIMER1_STOPWATCH_PRESCALER_DIVIDER prescaler tap.  Note that connecting
  // the clock source here probably allows the timer to run for a few
  // cycles before we reset everything and start handling interrupts, but
  // it shouldn't matter.
#if   TIMER1_STOPWATCH_PRESCALER_DIVIDER == 1
  TCCR1B |= _BV (CS10);
#elif TIMER1_STOPWATCH_PRESCALER_DIVIDER == 8
  TCCR1B |= _BV (CS11);
#elif TIMER1_STOPWATCH_PRESCALER_DIVIDER == 64
  TCCR1B |= _BV (CS11) | _BV (CS10);
#elif TIMER1_STOPWATCH_PRESCALER_DIVIDER == 256
  TCCR1B |= _BV (CS12);
#elif TIMER1_STOPWATCH_PRESCALER_DIVIDER == 1024
  TCCR1B |= _BV (CS12) | _BV (CS10);
#else
#  error invalid TIMER1_STOPWATCH_PRESCALER_DIVIDER setting
#endif

  TIMER1_STOPWATCH_RESET ();
}

void
timer1_stopwatch_shutdown (void)
{
  TIMSK1 &= ~(_BV (TOIE1));   // Disable overflow interrups for timer/counter1

  // Restore defaults for timer/counter1 control register B.  Note that this
  // will stop the timer.
  TCCR1B = TCCR1B_DEFAULT_VALUE;

  // Leave timer reading 0 as per interface promise
  TIMER1_STOPWATCH_RESET ();

  TCCR1A = TCCR1A_DEFAULT_VALUE;

  power_timer1_disable ();   // Shutdown timer/counter1 to save power
}
