// Generally useful stuff for AVR programming.

#ifndef UTIL_H
#define UTIL_H

#define HIGH 0x1
#define LOW  0x0

// FIXME: what about putting a blink blink routine in here for use as
// checkpoint, also maybe a trap point routine?

// WARNING: of course some contexts might understand things differently...
#define TRUE  0x1
#define FALSE 0x0

#define CLOCK_CYCLES_PER_MICROSECOND() (F_CPU / 1000000L)
#define CLOCK_CYCLES_TO_MICROSECONDS(a) (((a) * 1000L) / (F_CPU / 1000L))
#define MICROSECONDS_TO_CLOCK_CYCLES(a) (((a) * (F_CPU / 1000L)) / 1000L)

// Initialize and blink the LED on PB5 three quick times to indicate
// we're hit a checkpoint.  WARNING: no effort has been made to anticipate
// everything a client might have done to put PB5 in a mode where it can't
// be properly initialized/blinked.  Test this test function first :)
#define CHKP \
  do { \
    \
    DDRB |= _BV (DDB5); \
    loop_until_bit_is_set (DDRB, DDB5); \
    PORTB &= ~(_BV (PORTB5)); \
    \
    PORTB |= _BV (PORTB5); \
    _delay_ms (150); \
    PORTB &= ~(_BV (PORTB5)); \
    _delay_ms (150); \
    \
    PORTB |= _BV (PORTB5); \
    _delay_ms (150); \
    PORTB &= ~(_BV (PORTB5)); \
    _delay_ms (150); \
    \
    PORTB |= _BV (PORTB5); \
    _delay_ms (150); \
    PORTB &= ~(_BV (PORTB5)); \
    _delay_ms (150); \
  } while ( 0 )

#endif // UTIL_H
