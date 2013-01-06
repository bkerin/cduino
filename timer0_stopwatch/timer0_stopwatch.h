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
// For very precise timing of very short intervals of time, it will be more
// accurate to use busy waits, or to clear and read the value of TCNT0
// directly (the code for timer0_stopwatch() may be useful as an example
// of how to initialize the hardware).

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

// Do everything required to prepare the timer for use as an interrupt-driven
// stopwatch, in this order: 
//
//   * If the timer/counter0 hardware is shut down to save power,
//     enable it.
//
//   * Initialize the time/counter0 hardware to normal mode.
//
//   * Enable the prescaler as per TIMER0_STOPWATCH_PRESCALER_DIVIDER.
//
//   * Enable the timer/counter0 overflow interrupt source.
//
//   * Clear the overflow timer/counter0 overflow flag.
//
//   * Set the elapsed time to 0, and start it running.
//
//   * Ensure that interrupts are enabled globally.
void
timer0_stopwatch_init (void);

// Reset the timer/counter0 to 0.  All interrupts are deferred during
// execution of this routine.
void
timer0_stopwatch_reset (void);

// The type to use for the overflow counter.  This default value allows the
// stopwatch to run for abour 50 days (assuming a 16 MHz system clock), but
// see also TIMER0_STOPWATCH_RT for the probably more limiting readout type.
// There is little readout overhead using the default 32 bit type, which 64
// bit types impose significant overhead.  Its possible to specifiy other
// types at compile time; see the Makefile for this module.
#ifndef TIMER0_STOPWATCH_OCT
#  define TIMER0_STOPWATCH_OCT uint32_t
#endif
typedef TIMER0_STOPWATCH_OCT timer0_stopwatch_oct;

// The type to use for the timer readings.  This default value allows the
// stopwatch to run for about 4.7 hours (assuming a 16 MHz system clock).
// Its possible to specifiy other types at compile time; see the Makefile
// for this module.  This type should always be either exactly as wide as
// TIMER0_STOPWATCH_OCT, or one "type" wider (e.g. uint64_t if uint32_t is
// being used for the overflow counter).
#ifndef TIMER0_STOPWATCH_RT
#  define TIMER0_STOPWATCH_RT uint32_t
#endif
typedef TIMER0_STOPWATCH_RT timero_stopwatch_rt;

// This is the number of ticks we can measure without overflow,
// assuming the default 32 bit wide settings for TIMER0_STOPWATCH_OCT and
// TIMER0_STOPWATCH_RT.  If these values are changed this macro will be wrong.
// Note that either the overflow counter type or the readout type can be the
// limiting factor on this calculation.  If the types are the same width
// the readout type will be the limiting factor, otherwise the (narrower)
// overflow counter type will be the limiting factor.
#define TIMER0_STOPWATCH_OVERFLOW_TICKS_WITH UINT32_MAX

// Time from reset to overflow.
#define TIMER0_STOPWATCH_OVERFLOW_MICROSECONDS \
  ( \
    TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK * \
    TIMER0_STOPWATCH_OVERFLOW_TICKS \
  )

// Not intended for direct access: use an interface macro or function.
extern volatile timer0_stopwatch_oct timer0_stopwatch_oc;

// This is a conservative estimate of the per-call overhead associated with
// the TIMER0_STOPWATCH_TICKS() macro, in timer ticks (the actual measured
// overhead is about 9 ticks for me, but could depend on the compiler version,
// compilation options, etc.).

// This is the maximum per-use overhead associate with the
// TIMER0_STOPWATCH_TICKS() macro, assuming the default types for
// TIMER0_STOPWATCH_OCT and TIMER0_STOPWATCH_RT.  If other types are
// used, this value will be wrong.  I've also tried using 64 bit counter
// and readout types, and this seems to result in a *lot* more overhead
// (about 10 ticks for me).  Using a 16 bit type, on the otherhand, doesn't
// decrease overhead much.  This value has been determined experimentally
// (see timer0_stopwatch_test.c) but includes a safety margin and should
// be reliable unless the compiler does something really insane :)
#define TIMER0_STOPWATCH_TICKS_MACRO_MAX_OVERHEAD_TICKS 1

// Set OUTVAR (which must be a variable of type timer0_stopwatch_rt) to
// the current elapsed timer ticks.  This macro is provided because it can
// operate with a little bit less overhead than the timer0_stopwatch_ticks()
// function.  For explanations of how this macro works, see the implementation
// of that function.  FIXME: use __tcv or something here in case user has
// shadow warnings on?
#define TIMER0_STOPWATCH_TICKS(OUTVAR) \
  do { \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE) \
    { \
      uint8_t tcv = TCNT0; \
      \
      if ( TIFR0 & _BV (TOV0) ) { \
        OUTVAR \
          = (timer0_stopwatch_oc + 1) * TIMER0_STOPWATCH_COUNTER_VALUES; \
      } \
      else { \
        OUTVAR \
          = timer0_stopwatch_oc * TIMER0_STOPWATCH_COUNTER_VALUES + tcv; \
      } \
    } \
  } while ( 0 )

// This is the maximum per-use overhead associate with the
// timer0_stopwatch_ticks() function, assuming the default types for
// TIMER0_STOPWATCH_OCT and TIMER0_STOPWATCH_RT.  If other types are used,
// this value will be wrong.  I've also tried using 64 bit counter and
// readout types, and this seems to result in a *lot* more overhead (about
// 10 ticks for me).  Using a 16 bit type, on the otherhand, doesn't decrease
// overhead much.
#define TIMER0_STOPWATCH_TICKS_FUNCTION_MAX_OVERHEAD_TICKS 2

// Total number of timer/counter0 ticks since the last init() or reset()
// method call.  This routine is effectively atomic (All interrupts are
// deferred during most of its execution).
timer0_stopwatch_rt
timer0_stopwatch_ticks (void);

// The number of microseconds before results from
// timer0_stopwatch_microseconds() will overflow.
#define TIMER0_STOPWATCH_OVERFLOW_MICROSECONDS \
  TIMER0_STOPWATCH_OVERFLOW_TICKS * \
  TIMER0_STOPWATCH_MICROSECONDS_PER_TIMER_TICK 

// This macro is analagous to the
// TIMER0_STOPWATCH_TICKS_FUNCTION_MAX_READ_OVERHEAD_TICKS macro.
#define TIMER0_STOPWATCH_MICROSECONDS_FUNCTION_MAX_READ_OVERHEAD_US 4   

// The approximate number of elapsed microseconds since the last
// init() or reset() method call.  This is just a wrapper around the
// TIMER0_STOPWATCH_TICKS() macro.
timer0_stopwatch_rt
timer0_stopwatch_microseconds (void);

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
// NOTE that interrupts are NOT disabled globally (in that respect this
// routine is asymmetric with timer0_stopwatch_init()).
void
timer0_stopwatch_shutdown (void);

#endif // TIMER0_STOPWATCH_H
