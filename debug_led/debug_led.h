// Use a single LED to collect status or diagnostic information in various ways
//
// Test driver: debug_led_test.c    Implementation: debug_led.c
//
// Debugging with a LED can drive you mad.  But it can be effective in simple
// cases, and unlike approaches using other communication interfaces, it uses
// no other resources and it's super-fast and therefore much less prone to
// create heisenbergs.  But consider the debugging features available in
// term_io.h and wireless_xbee.h first.
//
// Bugs in debugging code can be particularly confusing.  You can trip
// yourself up by:
//
//   * Forgetting to call DBL_INIT().
//
//   * Setting the DBL_PIN to do something else (e.g. act as an input).
//
//   * Using a watchdog timer, and not defing DBL_FEED_WDT (but be careful
//     with that).
//
//   * Running out of RAM, especially inside a debugging routine itself.  The
//     results are undefined.
//
//   * Probably lots of other ways.  Verify the debugging routing itself.
//

#ifndef DEBUG_LED_H
#define DEBUG_LED_H

#include <inttypes.h>

#include "dio.h"

// If the client hasn't requested a particular pin, use PB5, since the
// Arduino has an on-board LED attached to that pin.
#ifndef DBL_PIN
#  define DBL_PIN DIO_PIN_PB5
#endif

// Call this first to initialize the pin connected to the debugging LED.
#define DBL_INIT() \
  do { DIO_INIT (DBL_PIN, DIO_OUTPUT, DIO_DONT_CARE, LOW); } while ( 0 )

// Turn the debugging LED on/off.  These macros provide a speedy
// heisenberg-resistant way to indicate a binary state.
#define DBL_ON() do { DIO_SET_HIGH (DBL_PIN); } while ( 0 )
#define DBL_OFF() do { DIO_SET_LOW (DBL_PIN); } while ( 0 )

// Blink count times, taking time_per_cycle milliseconds per on-off cycle.
// Iff DBL_FEED_WDT was defined at compile time, the watchdog timer (WDT)
// will be fed every 5 ms.  This is generally harmless if you aren't using
// the WDT, but often essential to LED-based debugging when you aren't.
// However, because its specifically designed to foil a safety reset
// mechanism, its not enabled by default.  The time_per_cycle argument
// should be greater than or equal to 10, and approximately a multiple of 10.
void
dbl_multiblink (uint16_t time_per_cycle, uint8_t count);

// When indicating a checkpoint, we will blink DBL_CHKP_BLINK_COUNT times,
// DBL_DHKP_BLINK_TIME milliseconds per on-off blink cycle.
#define DBL_CHKP_BLINK_TIME  300.0
#define DBL_CHKP_BLINK_COUNT 3

// Indicate a checkpoint (by blinking :).
#define DBL_CHKP() dbl_multiblink (DBL_CHKP_BLINK_TIME, DBL_CHKP_BLINK_COUNT)

// Period to use when blinking to indicate a trap point.
#define DBL_TRAP_POINT_BLINK_TIME 100.0

// Indicate that a trap has been hit by blinking rapidly forever.
#define DBL_TRAP() \
  do { dbl_multiblink (DBL_TRAP_POINT_BLINK_TIME, 1); } while ( TRUE )

// This routine is like assert(), but when condition is true we blink rapidly
// forever.  See DBL_ASSERT_SHOW_POINT() for a routine that actually gives
// code position feedback like assert().
#define DBL_ASSERT(condition) \
  do { if ( UNLIKELY (! (condition)) ) { DBL_TRAP (); } } while ( 0 )

// Well this ends up looking a lot like DBL_TRAP() doesn't it now :) For
// some reason my brain wanted them both.
#define DBL_ASSERT_NOT_REACHED() DBL_ASSERT (FALSE)

// "Display" uint32_t Value To Display.  The integer is represented using
// these steps:
//
//   1.  A short burst of rapid blinks is produced.
//
//   2.  The gvien value is printed to a string using printf format code
//       "%" PRIu32.
//
//   3.  For each digit in the string, a quick flash is produced for the
//       digit 0, or a series of up to 9 slower blinks corresponding to the
//       digit value is produced.
//
// This routine uses the definedness DBL_FEED_WDT to decide whether to
// feed the WDT or not, and won't get a chance to show much if a short WDT
// timeout is enabled and feeding isn't.
//
void
dbl_display_uint32 (uint32_t vtd);

// Like DBL_ASSERT(), but show the location of the assertion violation by
// first blinking out the number of characters in the source file name,
// then the line number of the violation using dbl_display_uint32().
//
// This routine uses the definedness DBL_FEED_WDT to decide whether to
// feed the WDT or not, and won't get a chance to show much if a short WDT
// timeout is enabled and feeding isn't.
#define DBL_ASSERT_SHOW_POINT(condition)        \
  do {                                          \
    if ( UNLIKELY (! (condition)) ) {           \
      for ( ; ; ) {                             \
        dbl_display_uint32 (strlen (__FILE__)); \
        dbl_display_uint32 (__LINE__);          \
      }                                         \
    }                                           \
  } while ( 0 )

#define DBL_ASSERT_NOT_REACHED_SHOW_POINT() DBL_ASSERT_SHOW_POINT (FALSE)

// If the user requests it, provide some namespace-busting short-form macros.
// NOTE that DBL_INIT() must be called as usual (no alias is provided for it).
#ifdef DBL_POLLUTE_NAMESPACE
#  define ON         DBL_ON
#  define OFF        DBL_OFF
// FIXME: for migration from when these were in util.h, we have this ifndef
#  ifndef CHKP
#    define CHKP       DBL_CHKP
#  endif
#  define TRAP       DBL_TRAP
#  define ASSERT     DBL_ASSERT
#  define ASSERTNR   DBL_ASSERT_NOT_REACHED
#  define ASSERTSP   DBL_ASSERT_SHOW_POINT
#  define ASSERTNRSP DBL_ASSERT_NOT_REACHED_SHOW_POINT
#endif

#endif // DEBUG_LED_H
