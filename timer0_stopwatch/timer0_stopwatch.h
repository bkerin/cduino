// Interface allowing timer/counter0 to be used as a stopwatch, by
// incrementing a software counter when a timer/counter0 overflow interrupt
// handler is triggered.  This interface essentially allows the eight bit
// timer to be used to precisely measure much longer periods of time.
//
// Note that this is NOT the interface to use for timer-driven alarms, output
// compare pin control, pulse width modulation, or other applications of
// the timer/counter0 hardware.  The hardware underlying this module can be
// used for variety of different purposes, and we don't try to support them
// all in one large, confusing interface.  Pick the software module that
// uses the hardware in the way you want (assuming its been written yet :).
//
// For very precise timing of very short intervals of time, it will be
// more accurate to use busy waits, or to clear and read the value of
// TCNT0 directly (the code for timer0_interrupt_driven_stopwatch()
// may be useful as an example of how to initialize the hardware).

#ifndef TIMER0_INTERRUPT_DRIVEN_STOPWATCH_H
#define TIMER0_INTERRUPT_DRIVEN_STOPWATCH_H

#include <stdint.h>
#include <util/atomic.h>

#include "util.h"

// The number of values the underlying counter can assums (values
// representable with eight bits).  Some interface macros need this, but
// there should be no reason to use it directly.
#define TIMER0_STOPWATCH_COUNTER_VALUES 256   

// This interface ensures that the prescaler divider is set as per this macro.
#define TIMER0_INTERRUPT_DRIVEN_STOPWATCH_PRESCALER_DIVIDER 64

// The number of microseconds per tick of the timer/counter0.
#define TIMER0_INTERRUPT_DRIVEN_STOPWATCH_MICROSECONDS_PER_TIMER_TICK \
  CLOCK_CYCLES_TO_MICROSECONDS \
    (TIMER0_INTERRUPT_DRIVEN_STOPWATCH_PRESCALER_DIVIDER)

// Do everything required to prepare the timer for use as an interrupt-driven
// stopwatch, in this order: 
//
//   * If the timer/counter0 hardware is shut down to save power,
//     enable it.
//
//   * Initialize the time/counter0 hardware to normal mode.
//
//   * Enable the prescaler as per
//     TIMER0_INTERRUPT_DRIVEN_STOPWATCH_PRESCALER_DIVIDER.
//
//   * Enable the timer/counter0 overflow interrupt source.
//
//   * Clear the overflow timer/counter0 overflow flag.
//
//   * Set the elapsed time to 0, and start it running.
//
//   * Ensure that interrupts are enabled globally.
void
timer0_interrupt_driven_stopwatch_init (void);

// Reset the timer/counter0 to 0.  All interrupts are deferred during
// execution of this routine.
void
timer0_interrupt_driven_stopwatch_reset (void);

// Not intended for direct access: use an interface macro or function.
extern volatile uint64_t timer0_overflow_count;

// Set OUTVAR (which must be a variable of type (FIXME: write in mutable
// type here)) to the current elapsed timer ticks.  This macro is provided
// because it can operate with a little bit less overhead than the
// timer0_stopwatch_ticks() function.  For explanations of how this macro
// works, see the implementation of that function.  FIXME: use __tcv or
// something here in case user has shadow warnings on?
#define TIMER0_STOPWATCH_TICKS(OUTVAR) \
  do { \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) \
    { \
      uint8_t tcv = TCNT0; \
      \
      if ( TIFR0 & _BV (TOV0) ) { \
        OUTVAR \
          = (timer0_overflow_count + 1) * TIMER0_STOPWATCH_COUNTER_VALUES; \
      } \
      else { \
        OUTVAR \
          = timer0_overflow_count * TIMER0_STOPWATCH_COUNTER_VALUES + tcv; \
      } \
    } \
  } while ( 0 )

// Total number of timer/counter0 ticks since the last init() or reset()
// method call.  This routine is effectively atomic (All interrupts are
// deferred during most of its execution).
uint64_t
timer0_interrupt_driven_stopwatch_ticks (void);

// The approximate number of elapsed microseconds since the last init() or
// reset() method call.  This should be about as precise as the underlying
// clock source, but it will take a few extra microseconds to make the
// computations involved.  This is mainly just a wrapper around the
// TIMER0_STOPWATCH_TICKS() macro.
uint64_t
timer0_interrupt_driven_stopwatch_microseconds (void);

// Stop timer/counter0 (saving power), restore the defaults for the
// timer/counter control registers, and disable the associated interrupt.
// The timer doesn't run after this method returns, and calls to the ticks()
// or microseconds() methods should always return 0.  Note that method
// leaves the timer/counter0 shutdown (PRTIM0 bit of the PRR register
// set to 1) to minimize power consumption.  It may not have been in this
// state before timer0_interrupt_driven_stopwatch_init() was first called.
// Note also that the global interrupt enable flag is not cleared by this
// function (even though the init() method does ensure that it is set).
void
timer0_interrupt_driven_stopwatch_shutdown (void);

#endif // TIMER0_INTERRUPT_DRIVEN_STOPWATCH_H
