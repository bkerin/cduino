// Generally useful stuff for AVR programming.

// vim:foldmethod=marker

#ifndef UTIL_H
#define UTIL_H

#include <avr/io.h>
#include <avr/version.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <util/atomic.h>
#include <util/delay.h>

// This version dependency applies to the whole library, this is just a
// convenient place to put it.
#if __AVR_LIBC_VERSION__ < 10800UL
#  error AVR Libc version 1.8.0 or later is required
#endif

// This library uses some GNU-specific features.
#ifndef __GNUC__
#  error __GNUC__ not defined
#endif

#define HIGH 0x01
#define LOW  0x00

// WARNING: of course some contexts might understand things differently.
// Fuse and lock bits read as zero when "programmed", for example.
#define TRUE  0x01
#define FALSE 0x00

// FIXME: I think these macros should return floats.  Its gross to require
// an explicit cast.  But that would require an audit of existing uses.
// Requiring the cast at least makes it clear when we can tolerate a
// float result.  WARNING: unless the argument a contains an explicit cast
// to double, the results of CLOCK_CYCLES_TO_MICROSECONDS() are subject
// to integer truncation.  WARNING: Only F_CPU values of greater that 1MHz
// with even multiples of 1MHz result in accurate settings for these macros.
// There are some alternate low-frequency macros which are pretty close,
// but haven't been tested.
#if F_CPU >= 1000000L
#  define CLOCK_CYCLES_PER_MICROSECOND() (F_CPU / 1000000L)
#  define CLOCK_CYCLES_TO_MICROSECONDS(a) \
     ((a) / CLOCK_CYCLES_PER_MICROSECOND())
#  define MICROSECONDS_TO_CLOCK_CYCLES(a) \
     ((a) * CLOCK_CYCLES_PER_MICROSECOND())
#else
#  error Interface untested with low-f clocks.  Remove #error and try :)
#  warning CLOCK_CYCLES_PER_MICROSECOND() would be less than 1 at this F_CPU
#  define CLOCK_CYCLES_TO_MICROSECONDS(a) (((a) * 1000L) / (F_CPU / 1000L))
#  define MICROSECONDS_TO_CLOCK_CYCLES(a) (((a) * (F_CPU / 1000L)) / 1000L)
#endif

// Branch prediction macros.  These let you hint the compiler whether a
// condition is likely to be true or not, so it can generate faster code.
#define LIKELY(condition)   __builtin_expect (!!(condition), 1)
#define UNLIKELY(condition) __builtin_expect (!!(condition), 0)

// Tracing and Debugging Info via LEDs or EEPROM

// The _delay_ms() and _delay_us() functions of AVR libc *REQUIRE* their
// arguments to be recognizable to GCC as double constants at compile-time.
// If they aren't various weird horrible effects can occur, including
// excessively long randomish delays, etc.  So for safety we have this
// functions as a way to get approximately correct largish delays.  Of course,
// using this for short delays would be insane.
#define DELAY_APPROX_MS(time)   \
  do {                          \
    double XxX_ems = 0.0;       \
    do {                        \
      _delay_ms (5.0);          \
      XxX_ems += 5.0;           \
    } while ( XxX_ems < time ); \
  } while ( 0 )

// NOTE: the CHKP() and BASSERT() macros are probably the most useful in
// this section.  The others let you change which pin drives the LED or
// do other fancy things.  To use a different pin, you generally have to
// do '#undef some_macro' then '#define some_macro' again with the DDRB,
// DDB5, PORTB, and PORB5 argument in the right hand side of the original
// definition replaced as appropriate in the new definition.

// Set pin for output low and toggle it high-low bc times, ~mspb ms per cycle.
// Useful for making LEDs blink See CHKP() for an example of how to call
// this macro.  WARNING: no effort has been made to anticipate everything a
// client might have done to put a pin in a state where it can't be properly
// initialized/blinked.  Test this test function first :)
#define CHKP_USING(ddr, ddrb, portr, portrb, mspb, bc)          \
  do {                                                          \
    ddr |= _BV (ddrb);                                          \
    loop_until_bit_is_set (ddr, ddrb);                          \
    portr &= ~(_BV (portrb));                                   \
    for ( uint8_t XxXcu_ii = 0 ; XxXcu_ii < bc ; XxXcu_ii++ ) { \
      portr |= _BV (portrb);                                    \
      DELAY_APPROX_MS (mspb / 2.0);                             \
      portr &= ~(_BV (portrb));                                 \
      DELAY_APPROX_MS (mspb / 2.0);                             \
    }                                                           \
  } while ( 0 )

// Form a trap point where we blink forever.
#define BTRAP_USING(ddr, ddrb, portr, portrb, mspb)                      \
  do { CHKP_USING (ddr, ddrb, portr, portrb, mspb, 1); } while ( TRUE )

// WARNING: some shields (e.g. shields which actively use SPI, the official
// Arduino motor shield, version R3) use the pin PB5 (AKA Digital 13 in
// Arduino-speak) for their own purposes, hence using this function will have
// unfortunate effects.  It can easily be redefined to toggle a different pin.
//
// Initialize and blink the on-board LED on PB5 three quick times to
// indicate that we've hit a checkpoint.  WARNING: no effort has been made
// to anticipate everything a client might have done to put PB5 in a state
// where it can't be properly initialized/blinked.  Test this test function
// first :)
#define CHKP() CHKP_USING(DDRB, DDB5, PORTB, PORTB5, 300.0, 3)

// Like CHKP(), but blinks a bit faster, forever.  The same caveats apply.
#define BTRAP() BTRAP_USING (DDRB, DDB5, PORTB, PORTB5, 100.0)

// Like assert(), but with frantic blinking :) The same caveats that apply
// to CHKP() apply to this macro.
#define BASSERT(condition)                                          \
  do { if ( UNLIKELY (! (condition)) ) { BTRAP (); } } while ( 0 )

// Do a busy wait for about Delay Time (dt) milliseconds while feeding the
// watchdog timer about every 5 milliseconds.  Useful for some assertion
// stuff we do and probably a sign of profound pathology everywhere else.
// Note that _delay_ms() requires an argument that GCC can identify as
// constant to work correctly.
#define DELAY_WHILE_FEEDING_WDT(dt)                                        \
  do {                                                                     \
    for ( uint16_t XxXdwfw_ii = 0 ; XxXdwfw_ii <= dt ; XxXdwfw_ii += 5 ) { \
      _delay_ms (5.0);                                                     \
      wdt_reset ();                                                        \
    }                                                                      \
  } while ( 0 )

// Like CHKP_USING(), but also calls wdt_reset() about every 5 ms.  Note that
// this routine is specifically designed to defeat the watchdog timer.
// Since it calls wdt_reset(), it might be required that the watchdog
// actually be in use (due to wdt_enable() having been called or the WDTON
// fuse being programmed), though this probably isn't really required.
#define CHKP_FEEDING_WDT_USING(ddr, ddrb, portr, portrb, mspb, bc)    \
  do {                                                                \
    ddr |= _BV (ddrb);                                                \
    loop_until_bit_is_set (ddr, ddrb);                                \
    portr &= ~(_BV (portrb));                                         \
                                                                      \
    for ( uint8_t XxXcfwu_ii = 0 ; XxXcfwu_ii < bc ; XxXcfwu_ii++ ) { \
      portr |= _BV (portrb);                                          \
      DELAY_WHILE_FEEDING_WDT (mspb / 2.0);                           \
      portr &= ~(_BV (portrb));                                       \
      DELAY_WHILE_FEEDING_WDT (mspb / 2.0);                           \
    }                                                                 \
  } while ( 0 )

// Like BTRAP_USING(), but also calls wdt_reset() about every 5 ms.  Note that
// this routine is specifically designed to defeat the watchdog timer.
#define BTRAP_FEEDING_WDT_USING(ddr, ddrb, portr, portrb, mspb) \
  do {                                                          \
    CHKP_FEEDING_WDT_USING (ddr, ddrb, portr, portrb, mspb, 1); \
  } while ( TRUE )

// Like CHKP(), but also calls wdt_reset() about every 5 ms.
// See CHKP_FEEDING_WDT_USING() for more details.
#define CHKP_FEEDING_WDT()                                    \
  CHKP_FEEDING_WDT_USING(DDRB, DDB5, PORTB, PORTB5, 300.0, 3)

// Like BTRAP(), but also calls wdt_reset() about every 5 ms.
// See CHKP_FEEDING_WDT_USING() for more details.
#define BTRAP_FEEDING_WDT() \
  BTRAP_FEEDING_WDT_USING (DDRB, DDB5, PORTB, PORTB5, 100.0)

// Like BASSERT(), but also calls wdt_reset() about every 5 ms.  Note that
// this will defeat the watchdog timer, forever.  I use it for debugging or
// when I have a failure requiring manual intervention to avoid endless resets
// and failure that might trash equipment.  See CHKP_FEEDING_WDT_USING()
// for more details.
#define BASSERT_FEEDING_WDT(condition) \
  do {                                 \
    if ( UNLIKELY (! (condition)) ) {  \
      BTRAP_FEEDING_WDT ();            \
    }                                  \
  } while ( 0 )

// Like CHKP_FEEDING_WDT_USING, but with the data direction and port related
// arguments fixed, but the time up to the client.  Possibly useful for
// making your own strange blink patterns, or for redefining such that
// BASSERT_FEEDING_WDT_SHOW_POINT() uses a LED other than one attached to PB5.
#define CHKP_FEEDING_WDT_WITH_TIME_AND_COUNT_ONLY(mspb, bc)   \
  CHKP_FEEDING_WDT_USING (DDRB, DDB5, PORTB, PORTB5, mspb, bc)

// Blink out a representation of an unsigned integer, calling wdt_reset()
// about every 5 ms as we go.  The integer is represented using these steps:
//
//   1.  A short burst of rapid blinks is produced.
//
//   2.  The given number is cast to type uint32_t, and printed to a string
//       using printf format code "%" PRIu32.
//
//   3.  For each digit in the string, a quick flash is produced for the
//       digit 0, or a series of 1-9 slower blinks corresponding to the
//       digit value is produced.
//
// WARNING: FIXXME: this should have a function core and macro wrapper to
// get the __FILE__ and __LINE__, it makes your program size explode, you
// can't afford to use it everywhere.  Same for some of the other blinky
// macros really.  But this is currently a sourceless header.
//
// Note that this macro requires CHKP_FEEDING_WDT_WITH_TIME_AND_COUNT_ONLY()
// to first be redefined as appropriate if PB5 isn't the one with the LED.
//
// That's all clients need to know.
//
// The weirdly named variables in this macro have the following meanings:
//
//   XxX_pbbb    Per-Blink-Batch Break
//   XxX_fbc     Fast Blink Count
//   XxX_fbp     Fast Blink Period
//   XxX_sbp     Slow Blink Period
//   lias        Length of (Int As String)
//
#define BLINK_OUT_UINT32_FEEDING_WDT(value)                                \
  do {                                                                     \
    uint16_t const XxX_pbbb = 942;                                         \
    uint16_t const XxX_fbp  = 100;                                         \
    uint8_t  const XxX_fbc  = 6;                                           \
    uint16_t const XxX_sbp  = 442;                                         \
    CHKP_FEEDING_WDT_WITH_TIME_AND_COUNT_ONLY (XxX_fbp, XxX_fbc);          \
    DELAY_WHILE_FEEDING_WDT (XxX_pbbb);                                    \
    char XxX_uint_as_string[11];                                           \
    sprintf (XxX_uint_as_string, "%" PRIu32, (uint32_t) value);            \
    uint8_t lias = strlen (XxX_uint_as_string);                            \
    for ( uint8_t XxXboifw_ii = 0 ; XxXboifw_ii < lias ; XxXboifw_ii++ ) { \
      uint8_t const ascii_0 = 48;                                          \
      uint8_t XxX_digit = XxX_uint_as_string[XxXboifw_ii] - ascii_0;       \
      if ( XxX_digit == 0 ) {                                              \
        CHKP_FEEDING_WDT_WITH_TIME_AND_COUNT_ONLY (XxX_fbp, 1);            \
      }                                                                    \
      else {                                                               \
        CHKP_FEEDING_WDT_WITH_TIME_AND_COUNT_ONLY (XxX_sbp, XxX_digit);    \
      }                                                                    \
      DELAY_WHILE_FEEDING_WDT (XxX_pbbb);                                  \
    }                                                                      \
  } while ( 0 )

// Like BASSERT_FEEDING_WDT, but attempts to show using a single LED where
// any assertion violation has occurred, using the following steps:
//
//   1. Call BLINK_OUT_UINT32_FEEDING_WDT(number_of_chars_in_source_file_name)
//
//   2. Call BLINK_OUT_UINT32_FEEDING_WDT(__LINE__)
//
//   3. Go to step 1
//
// WARNING: this macro suffers from the problem afflicting
// BLINK_OUT_UINT32_FEEDING_WDT().
//
// It's usually better to use term_io.h (see the TERM_IO_PTP macro
// in particular) or maybe wireless_xbee.h to sort out what's going on.
// But if you can't do that (perhaps because the serial port is being used
// to talk to something else) this can be useful.
//
#define BASSERT_FEEDING_WDT_SHOW_POINT(condition) \
  do {                                            \
    if ( UNLIKELY (! (condition)) ) {             \
      for ( ; ; ) {                               \
        size_t XxX_fnl = strlen (__FILE__);       \
        BLINK_OUT_UINT32_FEEDING_WDT (XxX_fnl);   \
        BLINK_OUT_UINT32_FEEDING_WDT (__LINE__);  \
      }                                           \
    }                                             \
  } while ( 0 )

// Offset from start of EEPROM used by LASSERT().
#define LASSERT_EEPROM_ADDRESS ((void *) 960)

// This is the RAM and EEPROM dedicated to the LASSERT() message.
#define LASSERT_BUFFER_SIZE 40

// Like assert(), but first stores the file and line of the violation
// at EEPROM address LASSER_EEPROM_ADDRESS.  It can be retrieved later
// with GET_LASSER_MESSAGE(), or cleared with CLEAR_LASSERT_MESSAGE().
// This may be useful for chasing down rare failures.
#define LASSERT(condition)                                                    \
  do {                                                                        \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)                                        \
    {                                                                         \
      if ( UNLIKELY (! (condition)) ) {                                       \
        char *XxX_msg = malloc (LASSERT_BUFFER_SIZE);                         \
        XxX_msg[0] = '\0';                                                    \
        int XxX_cp                                                            \
          = snprintf (                                                        \
              XxX_msg, LASSERT_BUFFER_SIZE, "%s: %i\n", __FILE__, __LINE__ ); \
        if ( XxX_cp < 0 ) {                                                   \
          XxX_msg = "error in LASSERT() itself";                              \
        }                                                                     \
        eeprom_update_block (                                                 \
            XxX_msg,                                                          \
            LASSERT_EEPROM_ADDRESS,                                           \
            strnlen (XxX_msg, LASSERT_BUFFER_SIZE) + 1 );                     \
        assert (0);                                                           \
      }                                                                       \
    }                                                                         \
  } while ( 0 )                                                               \

// Retrieve the last message stored by LASSERT() (or an empty string,
// if no LASSERT() violation has occurred since CLEAR_LASSERT_MESSAGE()
// was called or if EEPROM storage space doesn't contain a null byte).
// The buffer argument must point to LASSERT_BUFFER_SIZE bytes of memory.
#define GET_LASSERT_MESSAGE(buffer)                                          \
  do {                                                                       \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)                                       \
    {                                                                        \
      eeprom_read_block                                                      \
        (buffer, LASSERT_EEPROM_ADDRESS, LASSERT_BUFFER_SIZE);               \
      uint8_t XxX_got_null = FALSE;                                          \
      for ( uint8_t XxX_ii = 0 ; XxX_ii < LASSERT_BUFFER_SIZE ; XxX_ii++ ) { \
        if ( buffer[XxX_ii] == '\0' ) {                                      \
          XxX_got_null = TRUE;                                               \
          break;                                                             \
        }                                                                    \
      }                                                                      \
      if ( ! XxX_got_null ) {                                                \
        buffer[0] = '\0';                                                    \
      }                                                                      \
    }                                                                        \
  } while ( 0 )



// Clear any message stored in EEPROM by any previous LASSERT().
#define CLEAR_LASSERT_MESSAGE()                         \
  do {                                                  \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)                  \
    {                                                   \
      eeprom_write_byte (LASSERT_EEPROM_ADDRESS, '\0'); \
    }                                                   \
  } while ( 0 )

// }}}1

// All non-symbolic constants are evil :)
#define BITS_PER_BYTE 8

// Really, all of them :)
#define MS_PER_S  1000
#define US_PER_S  1000000
#define US_PER_MS (US_PER_S / MS_PER_S)

// Get the high or low byte of a two byte value (in a very paranoid way :)
#define HIGH_BYTE(two_byte_value)                                       \
  ((uint8_t) ((((uint16_t) two_byte_value) >> BITS_PER_BYTE) & 0x00ff))
#define LOW_BYTE(two_byte_value)                                        \
  ((uint8_t) (((uint16_t) two_byte_value) & 0x00ff))

// Stringify argument, i.e. return a string literal consisting of the
// unexpanded arg.
#define STRINGIFY(arg) #arg

// First expand arg, then STRINGIFY() it.
#define EXPAND_AND_STRINGIFY(arg) STRINGIFY (arg)

// Macros for more readable/writable binary bit patterns {{{1
#define B0 0
#define B00 0
#define B000 0
#define B0000 0
#define B00000 0
#define B000000 0
#define B0000000 0
#define B00000000 0
#define B1 1
#define B01 1
#define B001 1
#define B0001 1
#define B00001 1
#define B000001 1
#define B0000001 1
#define B00000001 1
#define B10 2
#define B010 2
#define B0010 2
#define B00010 2
#define B000010 2
#define B0000010 2
#define B00000010 2
#define B11 3
#define B011 3
#define B0011 3
#define B00011 3
#define B000011 3
#define B0000011 3
#define B00000011 3
#define B100 4
#define B0100 4
#define B00100 4
#define B000100 4
#define B0000100 4
#define B00000100 4
#define B101 5
#define B0101 5
#define B00101 5
#define B000101 5
#define B0000101 5
#define B00000101 5
#define B110 6
#define B0110 6
#define B00110 6
#define B000110 6
#define B0000110 6
#define B00000110 6
#define B111 7
#define B0111 7
#define B00111 7
#define B000111 7
#define B0000111 7
#define B00000111 7
#define B1000 8
#define B01000 8
#define B001000 8
#define B0001000 8
#define B00001000 8
#define B1001 9
#define B01001 9
#define B001001 9
#define B0001001 9
#define B00001001 9
#define B1010 10
#define B01010 10
#define B001010 10
#define B0001010 10
#define B00001010 10
#define B1011 11
#define B01011 11
#define B001011 11
#define B0001011 11
#define B00001011 11
#define B1100 12
#define B01100 12
#define B001100 12
#define B0001100 12
#define B00001100 12
#define B1101 13
#define B01101 13
#define B001101 13
#define B0001101 13
#define B00001101 13
#define B1110 14
#define B01110 14
#define B001110 14
#define B0001110 14
#define B00001110 14
#define B1111 15
#define B01111 15
#define B001111 15
#define B0001111 15
#define B00001111 15
#define B10000 16
#define B010000 16
#define B0010000 16
#define B00010000 16
#define B10001 17
#define B010001 17
#define B0010001 17
#define B00010001 17
#define B10010 18
#define B010010 18
#define B0010010 18
#define B00010010 18
#define B10011 19
#define B010011 19
#define B0010011 19
#define B00010011 19
#define B10100 20
#define B010100 20
#define B0010100 20
#define B00010100 20
#define B10101 21
#define B010101 21
#define B0010101 21
#define B00010101 21
#define B10110 22
#define B010110 22
#define B0010110 22
#define B00010110 22
#define B10111 23
#define B010111 23
#define B0010111 23
#define B00010111 23
#define B11000 24
#define B011000 24
#define B0011000 24
#define B00011000 24
#define B11001 25
#define B011001 25
#define B0011001 25
#define B00011001 25
#define B11010 26
#define B011010 26
#define B0011010 26
#define B00011010 26
#define B11011 27
#define B011011 27
#define B0011011 27
#define B00011011 27
#define B11100 28
#define B011100 28
#define B0011100 28
#define B00011100 28
#define B11101 29
#define B011101 29
#define B0011101 29
#define B00011101 29
#define B11110 30
#define B011110 30
#define B0011110 30
#define B00011110 30
#define B11111 31
#define B011111 31
#define B0011111 31
#define B00011111 31
#define B100000 32
#define B0100000 32
#define B00100000 32
#define B100001 33
#define B0100001 33
#define B00100001 33
#define B100010 34
#define B0100010 34
#define B00100010 34
#define B100011 35
#define B0100011 35
#define B00100011 35
#define B100100 36
#define B0100100 36
#define B00100100 36
#define B100101 37
#define B0100101 37
#define B00100101 37
#define B100110 38
#define B0100110 38
#define B00100110 38
#define B100111 39
#define B0100111 39
#define B00100111 39
#define B101000 40
#define B0101000 40
#define B00101000 40
#define B101001 41
#define B0101001 41
#define B00101001 41
#define B101010 42
#define B0101010 42
#define B00101010 42
#define B101011 43
#define B0101011 43
#define B00101011 43
#define B101100 44
#define B0101100 44
#define B00101100 44
#define B101101 45
#define B0101101 45
#define B00101101 45
#define B101110 46
#define B0101110 46
#define B00101110 46
#define B101111 47
#define B0101111 47
#define B00101111 47
#define B110000 48
#define B0110000 48
#define B00110000 48
#define B110001 49
#define B0110001 49
#define B00110001 49
#define B110010 50
#define B0110010 50
#define B00110010 50
#define B110011 51
#define B0110011 51
#define B00110011 51
#define B110100 52
#define B0110100 52
#define B00110100 52
#define B110101 53
#define B0110101 53
#define B00110101 53
#define B110110 54
#define B0110110 54
#define B00110110 54
#define B110111 55
#define B0110111 55
#define B00110111 55
#define B111000 56
#define B0111000 56
#define B00111000 56
#define B111001 57
#define B0111001 57
#define B00111001 57
#define B111010 58
#define B0111010 58
#define B00111010 58
#define B111011 59
#define B0111011 59
#define B00111011 59
#define B111100 60
#define B0111100 60
#define B00111100 60
#define B111101 61
#define B0111101 61
#define B00111101 61
#define B111110 62
#define B0111110 62
#define B00111110 62
#define B111111 63
#define B0111111 63
#define B00111111 63
#define B1000000 64
#define B01000000 64
#define B1000001 65
#define B01000001 65
#define B1000010 66
#define B01000010 66
#define B1000011 67
#define B01000011 67
#define B1000100 68
#define B01000100 68
#define B1000101 69
#define B01000101 69
#define B1000110 70
#define B01000110 70
#define B1000111 71
#define B01000111 71
#define B1001000 72
#define B01001000 72
#define B1001001 73
#define B01001001 73
#define B1001010 74
#define B01001010 74
#define B1001011 75
#define B01001011 75
#define B1001100 76
#define B01001100 76
#define B1001101 77
#define B01001101 77
#define B1001110 78
#define B01001110 78
#define B1001111 79
#define B01001111 79
#define B1010000 80
#define B01010000 80
#define B1010001 81
#define B01010001 81
#define B1010010 82
#define B01010010 82
#define B1010011 83
#define B01010011 83
#define B1010100 84
#define B01010100 84
#define B1010101 85
#define B01010101 85
#define B1010110 86
#define B01010110 86
#define B1010111 87
#define B01010111 87
#define B1011000 88
#define B01011000 88
#define B1011001 89
#define B01011001 89
#define B1011010 90
#define B01011010 90
#define B1011011 91
#define B01011011 91
#define B1011100 92
#define B01011100 92
#define B1011101 93
#define B01011101 93
#define B1011110 94
#define B01011110 94
#define B1011111 95
#define B01011111 95
#define B1100000 96
#define B01100000 96
#define B1100001 97
#define B01100001 97
#define B1100010 98
#define B01100010 98
#define B1100011 99
#define B01100011 99
#define B1100100 100
#define B01100100 100
#define B1100101 101
#define B01100101 101
#define B1100110 102
#define B01100110 102
#define B1100111 103
#define B01100111 103
#define B1101000 104
#define B01101000 104
#define B1101001 105
#define B01101001 105
#define B1101010 106
#define B01101010 106
#define B1101011 107
#define B01101011 107
#define B1101100 108
#define B01101100 108
#define B1101101 109
#define B01101101 109
#define B1101110 110
#define B01101110 110
#define B1101111 111
#define B01101111 111
#define B1110000 112
#define B01110000 112
#define B1110001 113
#define B01110001 113
#define B1110010 114
#define B01110010 114
#define B1110011 115
#define B01110011 115
#define B1110100 116
#define B01110100 116
#define B1110101 117
#define B01110101 117
#define B1110110 118
#define B01110110 118
#define B1110111 119
#define B01110111 119
#define B1111000 120
#define B01111000 120
#define B1111001 121
#define B01111001 121
#define B1111010 122
#define B01111010 122
#define B1111011 123
#define B01111011 123
#define B1111100 124
#define B01111100 124
#define B1111101 125
#define B01111101 125
#define B1111110 126
#define B01111110 126
#define B1111111 127
#define B01111111 127
#define B10000000 128
#define B10000001 129
#define B10000010 130
#define B10000011 131
#define B10000100 132
#define B10000101 133
#define B10000110 134
#define B10000111 135
#define B10001000 136
#define B10001001 137
#define B10001010 138
#define B10001011 139
#define B10001100 140
#define B10001101 141
#define B10001110 142
#define B10001111 143
#define B10010000 144
#define B10010001 145
#define B10010010 146
#define B10010011 147
#define B10010100 148
#define B10010101 149
#define B10010110 150
#define B10010111 151
#define B10011000 152
#define B10011001 153
#define B10011010 154
#define B10011011 155
#define B10011100 156
#define B10011101 157
#define B10011110 158
#define B10011111 159
#define B10100000 160
#define B10100001 161
#define B10100010 162
#define B10100011 163
#define B10100100 164
#define B10100101 165
#define B10100110 166
#define B10100111 167
#define B10101000 168
#define B10101001 169
#define B10101010 170
#define B10101011 171
#define B10101100 172
#define B10101101 173
#define B10101110 174
#define B10101111 175
#define B10110000 176
#define B10110001 177
#define B10110010 178
#define B10110011 179
#define B10110100 180
#define B10110101 181
#define B10110110 182
#define B10110111 183
#define B10111000 184
#define B10111001 185
#define B10111010 186
#define B10111011 187
#define B10111100 188
#define B10111101 189
#define B10111110 190
#define B10111111 191
#define B11000000 192
#define B11000001 193
#define B11000010 194
#define B11000011 195
#define B11000100 196
#define B11000101 197
#define B11000110 198
#define B11000111 199
#define B11001000 200
#define B11001001 201
#define B11001010 202
#define B11001011 203
#define B11001100 204
#define B11001101 205
#define B11001110 206
#define B11001111 207
#define B11010000 208
#define B11010001 209
#define B11010010 210
#define B11010011 211
#define B11010100 212
#define B11010101 213
#define B11010110 214
#define B11010111 215
#define B11011000 216
#define B11011001 217
#define B11011010 218
#define B11011011 219
#define B11011100 220
#define B11011101 221
#define B11011110 222
#define B11011111 223
#define B11100000 224
#define B11100001 225
#define B11100010 226
#define B11100011 227
#define B11100100 228
#define B11100101 229
#define B11100110 230
#define B11100111 231
#define B11101000 232
#define B11101001 233
#define B11101010 234
#define B11101011 235
#define B11101100 236
#define B11101101 237
#define B11101110 238
#define B11101111 239
#define B11110000 240
#define B11110001 241
#define B11110010 242
#define B11110011 243
#define B11110100 244
#define B11110101 245
#define B11110110 246
#define B11110111 247
#define B11111000 248
#define B11111001 249
#define B11111010 250
#define B11111011 251
#define B11111100 252
#define B11111101 253
#define B11111110 254
#define B11111111 255
// }}}1

#endif // UTIL_H
