// Interface to timer/counter0 and associated interrupts.  FIXME: actually,
// it may be a while before we get around to supporting the interrupts and
// such :).

#ifndef TIMER0_INTERRUPT_DRIVEN_COUNTER_H
#define TIMER0_INTERRUPT_DRIVEN_COUNTER_H

// This interface ensures that the prescaler divider
#define TIMER0_INTERRUPT_DRIVEN_COUNTER_PRESCALER_DIVIDER 64

timer0_interrupt_driven_counter_init (void)
{
// FIXME: veryify that the processor name macros work as expected then
// remove this.
#ifndef __AVR_ATmega328p__
  assert (0);
#endif

  PRR |= _BV (PRTIM0);   // Ensure timer0 not shut down to save power.

  // Ensure that the clock source for timer/counter is set to the
  // TIMER0_INTERRUPT_DRIVEN_COUNTER_PRESCALER_DIVIDER prescaler tap.
  TCCR0B &= ~(_BV (CS02));
  TCCR0B |= _BV (CS01) | _BV (CS00); 

  // Ensure that timer/counter0 is in normal mode (timer counts upwards and
  // simply overruns when it passes its maximum 8-bit value).
  TCCR0B &= ~(_BV (WGM02) | _BV (WGM01) | _BV (WGM00));
}

// Reset the timer/counter0 to 0.
void
timer0_interrupt_driven_counter_reset (void);

// Total number of timer/counter0 ticks since the last init() or reset()
// method call.
uint64_t
timer0_interrupt_driven_counter_ticks (void);

// The elapsed microseconds since the last init() or reset() method call.
uint64_t
timer0_interrupt_driven_counter_microseconds (void);

#endif // TIMER0_INTERRUPT_DRIVEN_COUNTER_H
