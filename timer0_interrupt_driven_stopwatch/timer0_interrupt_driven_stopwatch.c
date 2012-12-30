// Implementation of the interface described in
// timer0_interrupt_driven_stopwatch.c.

#include <avr/interrupt.h>
#include <util/atomic.h>
// FIXME: remove when done debugging
#include <util/delay.h>

#include "timer0_interrupt_driven_stopwatch.h"
#include "util.h"

#define TIMER0_VALUE_COUNT 256   // Values representable with eight bits

#ifndef __AVR_ATmega328P__
#  error This code has only been tested with the ATMega328p at 16 MHz on an \
         Arduino Uno board, though it should work with other ATMega or ATTiny \
         chips.
#endif

// FIXME: unexpose when done debug
volatile uint64_t timer0_overflow_count;
//static volatile uint64_t timer0_overflow_count;

// FIXME: debug schlop for tracking when we get into interrupt
volatile uint8_t hit_interrupt = 0;

// FIXME: debug schlop for tracking when we get into certain code
volatile uint8_t hit_ofc = 0;

// FIXME: remove debug schlop
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
  hit_interrupt++;
}

// Default values of the timer/counter0 control registers (for the ATMega328p
// at least), according to the datasheet.
#define TCCR0A_DEFAULT_VALUE 0x00
#define TCCR0B_DEFAULT_VALUE 0x00

void
timer0_interrupt_driven_stopwatch_init (void)
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
  TCNT0 = 0;
  // FIXME: must ensure the TOV0 is cleared here. It is cleared by writing
  // a logic one to the flag

  sei ();   // Enable interrupts.
}

void
timer0_interrupt_driven_stopwatch_reset (void)
{
  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
  {
    timer0_overflow_count = 0;
    TCNT0 = 0;
    // FIXME: must ensure the TOV0 is cleared here. It is cleared by writing
    // a logic one to the flag
  }
}

uint64_t
timer0_interrupt_driven_stopwatch_ticks (void)
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
timer0_interrupt_driven_stopwatch_microseconds (void)
{
  return 
    TIMER0_INTERRUPT_DRIVEN_STOPWATCH_MICROSECONDS_PER_TIMER_TICK 
    *  timer0_interrupt_driven_stopwatch_ticks ();
}

void
timer0_interrupt_driven_stopwatch_shutdown (void)
{
  TIMSK0 &= ~(_BV (TOIE0));   // Disable overflow interrups for timer/counter0.

  // Restore defaults for timer/counter0 control register B.  Note that this
  // will stop the timer.
  TCCR0B = TCCR0B_DEFAULT_VALUE;

  // Leave timer reading 0 as per interface promise.
  timer0_overflow_count = 0;
  TCNT0 = 0;
  // FIXME: must ensure the TOV0 is cleared here. It is cleared by writing
  // a logic one to the flag

  TCCR0A = TCCR0A_DEFAULT_VALUE;

  PRR &= ~(_BV (PRTIM0));   // Shutdown timer/counter0 to save power.
}
