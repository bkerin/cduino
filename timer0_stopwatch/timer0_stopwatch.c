// Implementation of the interface described in timer0_stopwatch.h.

#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/atomic.h>

#include "timer0_stopwatch.h"
#include "util.h"

#ifndef __AVR_ATmega328P__
#  error Processor macro __AVR_ATmega328P__ is not defined. This code has \
         only been tested with the ATMega328p at 16 MHz on an Arduino Uno \
         board, though it should work with other ATMega or ATTiny chips   \
         Use the source and relax this error trap :)
#endif

// FIXME: Make an "acknowledge" var or something for this (or just get rid
// of all the probably siller overhead guarantees).  The one_wire_slave module
// has a good example of an acknowledge setup for register-locked var use
#if F_CPU != 16000000
#  error F_CPU is not defined to be 16000000.  This code should work fine at \
         other frequencies (I have used it at 4 MHz), but the measured       \
         overhead values (*_OVERHEAD_* macros) will be wrong.  Just remove   \
         this error trap and enjoy :)
#endif

volatile uint32_t timer0_stopwatch_oc;

// Explicit support for ATTiny chip interrupt name thingies from AVR libc,
// to make migration to smaller/cheaper chips easier.
#if defined (__AVR_ATtiny24__) || \
    defined (__AVR_ATtiny44__) || \
    defined (__AVR_ATtiny84__)
ISR (TIM0_OVF_vect)
#else
ISR (TIMER0_OVF_vect)
#endif
{
  // Note that we don't need to use an atomic block here, as we're inside
  // an ordinary ISR block, so interrupts are globally deferred anyway.
  timer0_stopwatch_oc++;
}

// Default values of the timer/counter0 control registers (for the ATmega328P
// at least), according to the datasheet.
#define TCCR0A_DEFAULT_VALUE 0x00
#define TCCR0B_DEFAULT_VALUE 0x00

void
timer0_stopwatch_init (void)
{
  power_timer0_enable ();   // Ensure timer0 not shut down to save power

  // NOTE: these defaults correspond to the normal
  // count-up-overflow-at-the-top operation with all fancy optional timer
  // features disabled.
  TCCR0A = TCCR0A_DEFAULT_VALUE;
  TCCR0B = TCCR0B_DEFAULT_VALUE;

  // Reset the timer, in case it currently has some strange value that might
  // cause it to overflow (possibly triggering a deferred overflow interrupt)
  // as soon as we start it running.
  TCNT0 = 0;

  // Ensure that the clock source for timer/counter is set to the
  // TIMER0_STOPWATCH_PRESCALER_DIVIDER prescaler tap.  Note that connecting
  // the clock source here probably allows the timer to run for a few
  // cycles before we reset everything and start handling interrupts, but
  // it shouldn't matter.
  TCCR0B &= ~(_BV (CS02));
  TCCR0B |= _BV (CS01) | _BV (CS00);

  TIMSK0 |= _BV (TOIE0);   // Enable overflow interrupts for timer/counter0.

  timer0_stopwatch_reset ();

  sei ();   // Ensure that interrupts are enabled.
}

void
timer0_stopwatch_reset (void)
{
  // FIXME: potential clear-of TOV0 of TIFR0 here as there are for the
  // TIMER0_STOPWATCH_RESET_TCNT0() macro.
  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
  {
    timer0_stopwatch_oc = 0;

    // FIXME: this routine is now slightly longer than it was because of
    // the TSM use, the tests might be broken.  I think they had stupidly
    // tight timing guarantees anyway.
    //
    // FIXME: I think this common block of instructions should be factored
    // with the ones in the reset macro, could go in macro with _NONATOMIC
    // postfix or so.
    GTCCR |= _BV (TSM);
    GTCCR |= _BV (PSRSYNC);
    // Clear the overflow flag.  NOTE: it is my understanding that
    // clearing this will prevent any deferred overflow interrupt that
    // may have gone pending during this atomic block from executing:
    // see document "AVR130: Setup and Use the AVR Timers", section
    // "Example -- Timer0 Overflow Interrupt".  Note also that writing a
    // logic one to TOV1 actually *clears* it, and we don't have to use a
    // read-modify-write cycle to write the one (i.e. no "|=" required).
    // See http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits.
    TIFR0 = _BV (TOV0);
    TCNT0 = 0;
    GTCCR &= ~(_BV (TSM));
  }
}

uint32_t
timer0_stopwatch_ticks (void)
{
  uint32_t result;

  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
  {
    // Save timer/counter value in case it overflows while we're checking
    // for overflow (note that timers run parallel to everything, including
    // interrupt handlers).
    register uint8_t tcv = TCNT0;

    // If we have an uncleared overflow flag...
    if ( TIFR0 & _BV (TOV0) ) {
      // ...then we have one extra overflow that the interrup handler hasn't
      // had a chance to count yet, and it might even have happend since we
      // saved the value of TCNT0 a few instructions ago, so we don't add
      // tcv in.
      result = (timer0_stopwatch_oc + 1) * TIMER0_STOPWATCH_COUNTER_VALUES;
    }
    else {
      // Otherwise, the computation is as expected.
      result = timer0_stopwatch_oc * TIMER0_STOPWATCH_COUNTER_VALUES + tcv;
    }
  }

  return result;
}

uint32_t
timer0_stopwatch_microseconds (void)
{
  uint32_t tmp;
  TIMER0_STOPWATCH_TICKS (tmp);

  return TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK * tmp;
}

void
timer0_stopwatch_shutdown (void)
{
  TIMSK0 &= ~(_BV (TOIE0));   // Disable overflow interrups for timer/counter0

  // Restore defaults for timer/counter0 control register B.  Note that this
  // will stop the timer.
  TCCR0B = TCCR0B_DEFAULT_VALUE;

  // Leave timer reading 0 as per interface promise
  timer0_stopwatch_oc = 0;
  TCNT0 = 0;
  TIFR0 |= _BV (TOV0);   // Overflow flag is "cleared" by writing one to it

  TCCR0A = TCCR0A_DEFAULT_VALUE;

  power_timer0_disable ();   // Shutdown timer/counter0 to save power
}
