// I wrote this test program to help reassure myself that the pin
// change interrupts don't "miss" edges.  For example, if there's a fast
// high-low-high sequence, can we feel sure that if the high-low transition
// is caught, we'll also get a corresponding low-high transition?  In theory,
// the pin change detection can be designed such that if the high-low is
// caught, the hardware will notice if the pin end up high again, even
// if the following low-high was really fast.  And this appears to be the
// case, i.e. there is internal polling going on rather than just really
// fast edge detection in some other form.  Or at least, this test program
// never ended up with the wrong idea of the pin state despite me clicking
// messy little wires together a lot during its operation to ground the pin :)

#include <avr/interrupt.h>
#include <inttypes.h>
#include <util/delay.h>

#include "dio.h"
#include "term_io.h"

#define SWITCH_PIN            DIO_PIN_PB0
#define SWITCH_PIN_IC_REG     PCICR
#define SWITCH_PIN_IE_BIT     PCIE0
#define SWITCH_PIN_MSK_REG    PCMSK0
#define SWITCH_PIN_MSK_BIT    PCINT0
#define SWITCH_INTERRUPT_VECT PCINT0_vect

volatile uint8_t  sio = 1;   // Switch Is Open
volatile uint32_t ic  = 0;   // Interrupt Count


ISR (SWITCH_INTERRUPT_VECT)
{
  sio = DIO_READ (SWITCH_PIN);
  ic++;
  // Uncommenting this delay requires that ITERATIONS be decreased in order
  // for things to get done, but otherwise things still work.
  //_delay_ms (1.42);
}

#define DRIVE_LINE_LOW() \
  DIO_INIT (             \
      SWITCH_PIN,        \
      DIO_OUTPUT,        \
      DIO_DONT_CARE,     \
      LOW )

#define RELEASE_LINE()   \
  DIO_INIT (             \
      SWITCH_PIN,        \
      DIO_INPUT,         \
      DIO_ENABLE_PULLUP, \
      DIO_DONT_CARE )

// A delay can be introduced between the time the line is driven low and
// the time its released, and everything still works.
#define LOW_STINT()    \
  do {                 \
    DRIVE_LINE_LOW (); \
    RELEASE_LINE ();   \
  } while ( 0 );

int
main (void)
{
  term_io_init ();

  DIO_INIT (SWITCH_PIN, DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  SWITCH_PIN_IC_REG |= _BV (SWITCH_PIN_IE_BIT);
  loop_until_bit_is_set (SWITCH_PIN_IC_REG, SWITCH_PIN_IE_BIT);
  SWITCH_PIN_MSK_REG |= _BV (SWITCH_PIN_MSK_BIT);
  loop_until_bit_is_set (SWITCH_PIN_MSK_REG, SWITCH_PIN_MSK_BIT);

  sei ();

#define ITERATIONS 50000

  for ( ; ; ) {
    for ( uint32_t ii = 0 ; ii < ITERATIONS ; ii++ ) {
      _delay_ms (.02042);
      LOW_STINT ();
      _delay_ms (.02042);
    }
    // It also seems to work correctly in the simpler case where we never
    // drive the line low:
    //_delay_ms (2042.0);
    printf (
        "%" PRIu32 " interrupts.  Switch is %s\n",
        ic,
        (sio ? "open" : "closed") );
    ic = 0;

  }
}
