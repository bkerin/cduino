// Use timer/counter0 and an Interrupt to Measure Ellapsed Time
//
// Test driver: timer0_stopwatch_test.c    Implementation: timer0_stopwatch.c
//
// WARNING: many functions in this interface manipulate the presclar shared
// by timer/count0 and timer/counter1.  So they will affect the counting
// of timer/counter1.  If this is a problem, the implementation code can
// be edited to remove all statements that refer to bit PSRSYNC of GTCCR.
// This will leave timer1 alone, but adds one additional timer tick of
// uncertainty to measurements (since the current position of the prescaler
// will not be reset when the counter is reset).
//
// Interface allowing timer/counter0 to be used as a stopwatch, by
// incrementing a software overflow counter when a timer/counter0 overflow
// interrupt handler is triggered.  This arrangement allows the eight
// bit timer to be used to precisely measure much longer periods of time.
// There are also some macros to allow use of the raw counter value, without
// the overhead imposed by keeping track of the software overflow counter.
//
// Note that this is NOT the interface to use for timer-driven alarms, output
// compare pin control, pulse width modulation, or other applications of the
// timer/counter0 hardware.  The hardware underlying this module can be used
// for variety of different purposes, and we don't try to support them all
// in one large, confusing interface.  Pick the software module that uses
// the hardware in the way you want (assuming it has been written yet :).

#ifndef TIMER0_STOPWATCH_H
#define TIMER0_STOPWATCH_H

#include <stdint.h>
#include <util/atomic.h>

#include "util.h"

// The number of values the underlying counter can assums (values
// representable with eight bits).  Some interface macros need this, but
// there should be no reason to use it directly.
#define TIMER0_STOPWATCH_COUNTER_VALUES 256

// This interface ensures that the prescaler divider is set as per this macro.
// It should be possible to use a different prescaler setting, but many of
// the macros in this header which specify overflow and overhead performance
// will be incorrect.
#define TIMER0_STOPWATCH_PRESCALER_DIVIDER 64

// The number of microseconds per tick of the timer/counter0.
#define TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK \
  CLOCK_CYCLES_TO_MICROSECONDS (TIMER0_STOPWATCH_PRESCALER_DIVIDER)

// WARNING: this function manipulates the prescaler and thereby affects
// timer1 (which uses the same prescaler).
//
// Do everything required to prepare the timer for use as an interrupt-driven
// stopwatch, in this order:
//
//   * Ensure that the timer/counter0 hardware isn't shut down to save power.
//
//   * Initialize the time/counter0 hardware to normal mode, with OC0A and
//     OC0B disconnected.  This means TCCR0A dn TCCR0B are both set to all
//     zeros except for the clock select bits (CS02:0).
//
//   * Enable the prescaler as per TIMER0_STOPWATCH_PRESCALER_DIVIDER
//     (set CS02:0).
//
//   * Enable the timer/counter0 overflow interrupt source.
//
//   * Set our count of interrupt events (timer0_stopwatch_oc) to 0.
//
//   * Reset the stopwatch and start it running using timer0_stopwatch_init().
//
//   * Ensure that interrupts are enabled globally.
//
void
timer0_stopwatch_init (void);

// WARNING: this function stops and resets the prescaler and thereby
// affects the counting of the timer1 hardware (which shares the prescaler
// with timer0).  Reset prescaler and timer/counter0 to 0.  All interrupts
// are deferred during execution of this routine.
void
timer0_stopwatch_reset (void);

// This is the number of ticks we can measure without overflow.
#define TIMER0_STOPWATCH_OVERFLOW_TICKS UINT32_MAX

// An interface macro or function should be used to access this variable.
// This is the overflow counter that gets incremented in the interrupt handler
// when TCNT0 overflows.  Note: it's possible to use narrower or wider integer
// types here (and in the appropriate places in the implementation file).
// But there seems to be little advantage to doing so.  Using a 64 bit types
// results in a lot more overhead per read, and using a narrower type gives
// only a small reduction in overhead (see also TIMER0_STOPWATCH_TCNT0()
// if extremely high time precision is required).
extern volatile uint32_t timer0_stopwatch_oc;

// This is the maximum per-use overhead associate with the
// TIMER0_STOPWATCH_TICKS() macro.  This value has been determined
// experimentally (see timer0_stopwatch_test.c) but includes a safety margin
// and should be reliable unless the compiler does something really insane :)
#define TIMER0_STOPWATCH_TICKS_MACRO_MAX_OVERHEAD_TICKS 1

// Set OUTVAR (which must be a variable of type uint32_t) to the current
// elapsed timer ticks.  This macro is provided because it can operate with a
// little bit less time overhead than the timer0_stopwatch_ticks() function
// (at least when the compiler is set to optimize for small code size).
// For explanations of how this macro works, see the implementation of
// that function in timer0_stopwatch.c.
#define TIMER0_STOPWATCH_TICKS(OUTVAR) \
  do { \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) \
    { \
      register uint8_t XxX_tcv = TCNT0; \
      \
      if ( TIFR0 & _BV (TOV0) ) { \
        OUTVAR \
          = (timer0_stopwatch_oc + 1) * TIMER0_STOPWATCH_COUNTER_VALUES; \
      } \
      else { \
        OUTVAR \
          = timer0_stopwatch_oc * TIMER0_STOPWATCH_COUNTER_VALUES + XxX_tcv; \
      } \
    } \
  } while ( 0 )

// This is the maximum per-use overhead associate with the
// timer0_stopwatch_ticks() function.  This value has been determined
// experimentally (see timer0_stopwatch_test.c) but includes a safety margin
// and should be reliable unless the compiler does something really insane :)
#define TIMER0_STOPWATCH_TICKS_FUNCTION_MAX_OVERHEAD_TICKS 1

// Total number of timer/counter0 ticks since the last init() or reset()
// method call.  This routine is effectively atomic (All interrupts are
// deferred during most of its execution).
uint32_t
timer0_stopwatch_ticks (void);

// The number of microseconds before results from
// timer0_stopwatch_microseconds() will overflow.
#define TIMER0_STOPWATCH_OVERFLOW_MICROSECONDS \
  ( \
    TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK * \
    TIMER0_STOPWATCH_OVERFLOW_TICKS \
  )

// This macro is analagous to the
// TIMER0_STOPWATCH_TICKS_FUNCTION_MAX_READ_OVERHEAD_TICKS macro.
#define TIMER0_STOPWATCH_MICROSECONDS_FUNCTION_MAX_READ_OVERHEAD_US 4

// The approximate number of elapsed microseconds since the last
// init() or reset() method call.  This is just a wrapper around the
// TIMER0_STOPWATCH_TICKS() macro.
uint32_t
timer0_stopwatch_microseconds (void);

// WARNING: this macro resets the prescaler and thereby affects the counting
// of the timer1 hardware (which shares the prescaler with timer0).
// This macro can be used together with TIMER0_STOPWATCH_TCNT0 to time
// very short intervals of time with minimal overhead.  It doesn't reset
// the overflow counter and isn't appropriate for timing intervals
// of time long enough for overflow of the eight bit TCNT0 to occur.
// See the timer0_stopwatch_reset implementation for explanation of the
// individual instructions.  The time between the completion of this code
// and the evaluation of TIMER0_STOPWATCH_TCNT0 in an immediately following
// statement should not be more than a couple of machine instructions.
// About the only thing you can do to get tighter timing performance is
// to disable the timer overflow interrupt, so that you don't have to
// worry about clearing TOV0.  This interface doesn't support doing that,
// however.  Note that the stopwatch only begins running at the end of
// this sequence, when TSM is written to zero.  Note also that writing
// a logic one to TOV1 actually *clears* it, and we don't have to use
// a read-modify-write cycle to write the one (i.e. no "|=" required).
// See http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits.
// FIXME: should test this guy again now we do it like in timer1_stopwatch.
// FIXME: the two GTCCR bits we set could probably be done in one assign to
// save an extra read-modify-write, but I'm not sure how I would test it.
// Same thing in timer1.
#define TIMER0_STOPWATCH_RESET_TCNT0() \
  do {                                 \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) \
    {                                  \
      GTCCR |= _BV (TSM);              \
      GTCCR |= _BV (PSRSYNC);          \
      TIFR0 = _BV (TOV0);              \
      TCNT0 = 0;                       \
      GTCCR &= ~(_BV (TSM));           \
    }                                  \
  } while ( 0 )

// This macro evaluates to the current value of the counter.  It should be
// used together with TIMER0_STOPWATCH_RESET_TCNT0().  See the description
// of that macro for more details.
#define TIMER0_STOPWATCH_TCNT0() TCNT0

// This method entirely shuts down timer/counter0:
//
//   * The timer/counter0 overflow interrupt is disabled.
//
//   * The timer/counter0 control registers TCCR0A and TCCR0B are reset to
//     their default values.
//
//   * The overflow flag is cleared.
//
//   * The timer reading is reset to 0.
//
//   * Th counter is entirely disabled to save power.
//
// NOTE that interrupts are NOT disabled globally (in this respect this
// routine is asymmetric with timer0_stopwatch_init()).
void
timer0_stopwatch_shutdown (void);

#endif // TIMER0_STOPWATCH_H
