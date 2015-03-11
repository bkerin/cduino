// Implementation of the interface described in debug_led.h

// Do a busy wait for about Delay Time (dt) milliseconds while feeding the
// watchdog timer about every 5 milliseconds.  Useful for some assertion
// stuff we do and probably a sign of profound pathology everywhere else.
// Note that _delay_ms() requires an argument that GCC can identify as
// constant to work correctly.

#include "debug_led.h"

#ifdef DBL_FEED_WDT
#  define MAYBE_FEED_WDT() wdt_reset ()
#else
#  define MAYBE_FEED_WDT() do { ; } while ( 0 )
#endif

// Delay Time Atom Size.  See the context below.
#define DTAS 5

// FIXME: possibly all malloc() use should probably go away, since it might
// cause memory fragmentation and weird bugs

// The _delay_ms() and _delay_us() functions of AVR libc *REQUIRE* their
// arguments to be recognizable to GCC as double constants at compile-time.
// If they aren't various weird horrible effects can occur, including
// excessively long randomish delays, etc.  So for safety we have this
// macro as a way to get approximately correct largish delays.  Of course,
// using this for short delays would be insane.  It wastes about 25 bytes
// of flash to make this a macro rather than a function, but it should save
// a little bit of stack which is good for debugging functions.
#define DELAY_APPROX(time_ms)                                          \
  do {                                                                 \
    for ( uint16_t XxX_ii = 0 ; XxX_ii <= time_ms ; XxX_ii += DTAS ) { \
      _delay_ms (DTAS);                                                \
      MAYBE_FEED_WDT ();                                               \
    }                                                                  \
  } while ( 0 )

void
dbl_multiblink (uint16_t time_per_cycle, uint8_t count)
{
  for ( uint8_t ii = 0 ; ii < count ; ii++ ) {
    DBL_ON ();
    DELAY_APPROX (time_per_cycle / 2);
    DBL_OFF ();
    DELAY_APPROX (time_per_cycle / 2);
  }
}

// This is just big enough to hold all the digits of a uint32_t and a
// trailing nul byte.
#define STRING_BUFFER_SIZE 11

void
dbl_display_uint32 (uint32_t vtd)
{
  uint16_t const pbbb = 942;   // Per-Blink-Batch Break
  uint16_t const fbp  = 100;   // Fast Blink Count
  uint8_t  const fbc  = 6;     // Fast Blink Period
  uint16_t const sbp  = 442;   // Slow Blink Period

  dbl_multiblink (fbp, fbc);
  DELAY_APPROX (pbbb);
  char uint_as_string[STRING_BUFFER_SIZE];
  sprintf (uint_as_string, "%" PRIu32, (uint32_t) vtd);
  uint8_t lias = strlen (uint_as_string);   // Length of Int As String
  for ( uint8_t ii = 0 ; ii < lias ; ii++ ) {
    // Take advantage of the fact that ASCII digit number are in order
    uint8_t const ascii_0 = 48;
    uint8_t digit = uint_as_string[ii] - ascii_0;
    if ( digit == 0 ) {
      dbl_multiblink (fbp, 1);
    }
    else {
      dbl_multiblink (sbp, digit);
    }
    DELAY_APPROX (pbbb);
  }
}
