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

#include "util.h"

// This interface ensures that the prescaler divider is set as per this macro.
#define TIMER0_INTERRUPT_DRIVEN_STOPWATCH_PRESCALER_DIVIDER 64

// The numer of microseconds per tick of the timer/counter0.
#define TIMER0_INTERRUPT_DRIVEN_STOPWATCH_MICROSECONDS_PER_TIMER_TICK \
  CLOCK_CYCLES_TO_MICROSECONDS \
    (TIMER0_INTERRUPT_DRIVEN_STOPWATCH_PRESCALER_DIVIDER)

extern volatile uint8_t hit_interrupt;
extern volatile uint8_t hit_ofc;
extern volatile uint64_t timer0_overflow_count;

// Initialize the time/counter0 hardware, install
// the interrupt handler, enable the prescaler as per
// TIMER0_INTERRUPT_DRIVEN_STOPWATCH_PRESCALER_DIVIDER, set the elapsed
// time to 0, enable timer/counter0 overflow interrupts, and ensure that
// interrupts are enabled globally (call sei()).  If the timer/counter0
// hardware is shut down to save power (PRTIM0 bit of the PRR register is
// set to 1), this routine enables it (PRTIM0 gets set to 0).
void
timer0_interrupt_driven_stopwatch_init (void);

// Reset the timer/counter0 to 0.  All interrupts are deferred during
// execution of this routine.
void
timer0_interrupt_driven_stopwatch_reset (void);

// Total number of timer/counter0 ticks since the last init() or reset()
// method call.  All interrupts are deferred during most of the execution
// of this routine.
uint64_t
timer0_interrupt_driven_stopwatch_ticks (void);

// The approximate number of elapsed microseconds since the last init() or
// reset() method call.  This should be about as precise as the underlying
// clock source, but it will take a few extra microseconds to make the
// computations involved.  All interrupts are deferred during most of the
// execution of this routine.
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
