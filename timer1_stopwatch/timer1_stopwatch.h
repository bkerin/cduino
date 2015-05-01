// Use timer/counter1 to Measure Ellapsed Time
//
// Test driver: timer1_stopwatch_test.c    Implementation: timer1_stopwatch.c
//
// WARNING: many functions in this interface manipulate the presclar shared
// by timer/counter0 and timer/counter1.  So they will affect the counting
// of timer/counter0.  If this is a problem, the implementation code can
// be edited to remove all statements that refer to bit PSRSYNC of GTCCR.
// This will leave timer0 alone, but adds one additional timer tick of
// uncertainty to measurements (since the current position of the prescaler
// will not be reset when the counter is reset).
//
// Interface allowing timer/counter1 to be used as a stopwatch for short
// periods of time.  Unlike the timer0_stopwatch.h interface, this one
// doesn't use an interrupt at all.  Timer1 is a 16 bit timer, so you can
// measure a decent chunk of time without all the complexity of automatic
// overflow counting.
//
// Note that this is NOT the interface to use for timer-driven alarms, output
// compare pin control, pulse width modulation, or other applications of the
// timer/counter1 hardware.  The hardware underlying this module can be used
// for variety of different purposes, and we don't try to support them all
// in one large, confusing interface.  Pick the software module that uses
// the hardware in the way you want (assuming it has been written yet :).

#ifndef TIMER1_STOPWATCH_H
#define TIMER1_STOPWATCH_H

#include <stdint.h>
#include <util/atomic.h>

#include "util.h"

// Provide a default value for the prescaler divider.  Other possible settings
// are 1, 8, 256, and 1024.  WARNING: if you want to set this to something
// different, you must do so in the module Makefile or from the make command
// line, since you want *all* inclusions of this header (including the one
// in one_wire_master.c) to see the same value.  Doing otherwise invites
// awful bugs in which different parts of the code have different ideas of
// how long timer ticks are.
#ifndef TIMER1_STOPWATCH_PRESCALER_DIVIDER
#  define TIMER1_STOPWATCH_PRESCALER_DIVIDER 64
#endif

#if F_CPU < 1000000
#  error F_CPU is less than 1 MHz.  This module uses a macro from util.h that \
         probably does not work right at cpu frequencies this low.  Making it \
         work is probably trivial, start by removing this error trap and \
         letting the similar errors in util.h go off, then remove the simple \
         dependencies on the problematic macro from that file.
#endif

// The number of values the underlying counter can assume.  Some interface
// macros need this, but there should be no reason to use it directly.
#define TIMER1_STOPWATCH_COUNTER_VALUES (((uint32_t) UINT16_MAX) + 1)

// The number of microseconds per tick of the timer/counter1.
#define TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK \
  CLOCK_CYCLES_TO_MICROSECONDS (((double) TIMER1_STOPWATCH_PRESCALER_DIVIDER))

// The number of microseconds before timer/counter1 will overflow.
#define TIMER1_STOPWATCH_OVERFLOW_MICROSECONDS \
  ( \
    TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK * \
    TIMER1_STOPWATCH_COUNTER_VALUES \
  )

// WARNING: this function manipulates the prescaler and thereby affects
// timer0 (which uses the same prescaler).
//
// Do everything required to prepare the timer for use as a stopwatch,
// in this order:
//
//   * Ensure that the timer/counter1 hardware isn't shut down to save power.
//
//   * Initialize the time/counter1 hardware to normal mode, with OC1A and
//     OC1B disconnected.  This means TCCR1A and TCCR1B both end upset to
//     all zeros except for the clock select bits (CS12:0).
//
//   * Initialize the prescaler as per TIMER1_STOPWATCH_PRESCALER_DIVIDER
//     (set CS12:0).
//
//   * Reset the stopwatch and start it running using TIMER1_STOPWATCH_RESET().
//
void
timer1_stopwatch_init (void);

// WARNING: this macro halts and resets the prescaler and thereby affects the
// counting of the timer0 hardware (which shares the prescaler with timer1).
// Note that the use of timer sychronization mode (TSM bit of GTCCR register)
// means that the prescaler is stopped, which means that timer0 might lose
// quite a bit of time if you have many interrupts or something.  You might
// want to use an AVR libc ATOMIC_BLOCK(ATOMIC_RESTORESTATE) around this
// macro in this situation.  If your program writes *or reads* TCNT1 from
// an interrupt service routine you *must* use an atomic block around this
// macro outside that routine; see the comments for TIMER1_STOPWATCH_TICKS()
// below.  Note that the stopwatch only begins running at the end of
// this sequence, when TSM is written to zero.  Note also that writing
// a logic one to TOV1 actually *clears* it, and we don't have to use
// a read-modify-write cycle to write the one (i.e. no "|=" required).
// See http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits.
#define TIMER1_STOPWATCH_RESET() \
  do {                           \
    GTCCR |= _BV (TSM);          \
    GTCCR |= _BV (PSRSYNC);      \
    TIFR1 = _BV (TOV1);          \
    TCNT1 = 0;                   \
    GTCCR &= ~(_BV (TSM));       \
  } while ( 0 )

// This does a quick but sloppy reset.  The timer is not stopped
// or synchronized with the presaler, so for larger presaler settings
// considerable inaccuracy may be involved in the subsequent time measurement.
// The overflow flag bit TOV1 of the Timer/Counter1 Interrupt Flag Register
// TIFR1 is not cleared.
#define TIMER1_STOPWATCH_FAST_RESET() \
  do {                                \
    TCNT1 = 0;                        \
  } while ( 0 )

// Number of ticks since timer/counter1 was last reset or overflowed.  NOTE:
// if this macro (or TCNT1 via any other mechanism) will ever be written *or
// read* from an interrupt service routine, then an AVR libc ATOMIC_BLOCK
// (from the AVR libc header util/atomic.h) must be used around the access
// in the main thread at least (the interrupt handler is probably atomic
// anyway, since interrupts are normally disable while other interrup
// handlers are running).  Note that even if there is no possibility of a
// write to this register, read corruption can still occur, because a shared
// internal temporary register is used to read the 16 bit timer value.
// See the ATmega328P datasheet Revision 8271C, section 15.3.
#define TIMER1_STOPWATCH_TICKS() TCNT1

// Number of microseconds since timer/counter1 was last reset or overflowed.
// The same considerations that apply to TIMER1_STOPWATCH_TICKS() apply to
// this macro.
#define TIMER1_STOPWATCH_MICROSECONDS() \
  (TIMER1_STOPWATCH_TICKS() * TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK)

// This macro evaluates to true iff the timer has overflowed since it was
// last reset.
#define TIMER1_STOPWATCH_OVERFLOWED() (TIFR1 & _BV (TOV1))

// This method entirely shuts down timer/counter1:
//
//   * The timer/counter1 overflow interrupt is disabled (though this
//     interface never enabled it in the first place).
//
//   * The timer/counter1 control registers TCCR1A and TCCR1B are reset to
//     their default values.
//
//   * The overflow flag is cleared.
//
//   * The timer reading is reset to 0.
//
//   * The counter is entirely disabled to save power.
//
void
timer1_stopwatch_shutdown (void);

#endif // TIMER1_STOPWATCH_H
