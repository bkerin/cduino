// Implementation of the interface described in timer0_stopwatch.c.

#include <avr/interrupt.h>
#include <util/atomic.h>

#include "timer0_stopwatch.h"
#include "util.h"

#define TIMER0_VALUE_COUNT 256   // Values representable with eight bits

#ifndef __AVR_ATmega328P__
#  error This code has only been tested with the ATMega328p at 16 MHz on an \
         Arduino Uno board, though it should work with other ATMega or ATTiny \
         chips.
#endif

// FIXME: make this type definable at compile time, so we can see if we
// get fewer ticks of overhead between calls with a smaller type.
volatile uint64_t timer0_overflow_count;

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
  timer0_overflow_count++;
}

// Default values of the timer/counter0 control registers (for the ATMega328p
// at least), according to the datasheet.
#define TCCR0A_DEFAULT_VALUE 0x00
#define TCCR0B_DEFAULT_VALUE 0x00

void
timer0_stopwatch_init (void)
{
  PRR &= ~(_BV (PRTIM0));   // Ensure timer0 not shut down to save power.

  TCCR0A = TCCR0A_DEFAULT_VALUE;
  TCCR0B = TCCR0B_DEFAULT_VALUE;

  // Ensure that timer/counter0 is in normal mode (timer counts upwards and
  // simply overruns when it passes its maximum 8-bit value).
  TCCR0B &= ~(_BV (WGM02) | _BV (WGM01) | _BV (WGM00));

  // Ensure that the clock source for timer/counter is set to the
  // TIMER0_INTERRUPT_DRIVEN_STOPWATCH_PRESCALER_DIVIDER prescaler tap.
  TCCR0B &= ~(_BV (CS02));
  TCCR0B |= _BV (CS01) | _BV (CS00); 

  TIMSK0 |= _BV (TOIE0);   // Enable overflow interrupts for timer/counter0.

  timer0_overflow_count = 0;
  TIFR0 |= _BV (TOV0);   // Overflow flag is "cleared" by writing one to it
  TCNT0 = 0;

  sei ();   // Enable interrupts.
}

void
timer0_stopwatch_reset (void)
{
  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
  {
    timer0_overflow_count = 0;
    TIFR0 |= _BV (TOV0);   // Overflow flag is "cleared" by writing one to it
    TCNT0 = 0;
  }
}

uint64_t
timer0_stopwatch_ticks (void)
{
  uint64_t result;

  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
  {
    // Save timer/counter value in case it overflows while we're checking
    // for overflow (note that timers run parallel to everything, including
    // interrupt handlers).
    uint8_t tcv = TCNT0;  

    // If we have an uncleared overflow flag...
    if ( TIFR0 & _BV (TOV0) ) {
      // ...then we have one extra overflow that the interrup handler hasn't
      // had a chance to count yet, and it might even have happend since we
      // saved hte value of TCNT0 a few instructions ago, so we don't add
      // tcv in.
      result = (timer0_overflow_count + 1) * TIMER0_VALUE_COUNT;
    }
    else {
      // Otherwise, the computation is as expected.
      result = timer0_overflow_count * TIMER0_VALUE_COUNT + tcv; 
    }
  }

  return result;
}

uint64_t
timer0_stopwatch_microseconds (void)
{
  uint64_t tmp;
  TIMER0_STOPWATCH_TICKS (tmp);
  return TIMER0_INTERRUPT_DRIVEN_STOPWATCH_MICROSECONDS_PER_TIMER_TICK * tmp;
}

// FIXME: need to add a note about the possible delay you get at the start of
// timing from prescaler not being reset.  Or else reset the prescaler, or add
// an option to do that.  But then we might interact with other timers using
// the same prescaler, which would be annoying, so it should be an option.

void
timer0_stopwatch_shutdown (void)
{
  TIMSK0 &= ~(_BV (TOIE0));   // Disable overflow interrups for timer/counter0.

  // Restore defaults for timer/counter0 control register B.  Note that this
  // will stop the timer.
  TCCR0B = TCCR0B_DEFAULT_VALUE;

  // Leave timer reading 0 as per interface promise.
  timer0_overflow_count = 0;
  TCNT0 = 0;
  TIFR0 |= _BV (TOV0);   // Overflow flag is "cleared" by writing one to it

  TCCR0A = TCCR0A_DEFAULT_VALUE;

  PRR &= ~(_BV (PRTIM0));   // Shutdown timer/counter0 to save power.
}
