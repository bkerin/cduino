// Implementation of the interface described in
// timer0_interrupt_driven_stopwatch.c.

#include "timer0_interrupt_driven_stopwatch.h"

#include "wiring_private.h"

#define TIMER0_VALUE_COUNT 256   // Values representable with eight bits

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW \
  (CLOCK_CYCLES_TO_MICROSECONDS \
    (TIMER0_INTERRUPT_DRIVEN_COUNTER_PRESCALER_DIVIDER * TIMER0_VALUE_COUNT))

// The whole number of milliseconds per counter0 overflow.
#define MILLISECONDS_INCREMENT (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// The fractional number of milliseconds per counter0 overflow.  We shift
// right by three to fit these numbers into a byte. (for the clock speeds
// we care about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_milliseconds = 0;
static unsigned char timer0_fract = 0;

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
  // Copy these to local variables so they can be stored in registers
  // (volatile variables must be read from memory on every access).
  unsigned long m = timer0_millis;
  unsigned char f = timer0_fract;

  m += MILLIS_INC;
  f += FRACT_INC;
  if (f >= FRACT_MAX) {
    f -= FRACT_MAX;
    m += 1;
  }

  timer0_fract = f;
  timer0_millis = m;
  timer0_overflow_count++;
}

void
timer0_interrupt_driven_counter_reset (void)
{
  // FIXME: fill in.  
}

uint16_t
timer0_interrupt_driven_counter_microseconds (void)
{
  unsigned long milliseconds;
  uint8_t old_sreg = SREG;

  // Disable interrupts while we read timer0_millis or we might get an
  // inconsistent value (e.g. in the middle of a write to timer0_millis).
  // FIXME: should we use one of the atomic macros from AVR libc here?  FIXME:
  // what units should we use, and what width datatype.  The above code in the
  // interrupt handler makes a big deal of using local copies so registers
  // can be used but I'm not sure how sensible this is, and anyway we could
  // use a helper variable and a 64 bit type into which it gets accumulated.

  cli ();
  milliseconds = timer0_micros;
  SREG = old_sreg;

  return m;
}
