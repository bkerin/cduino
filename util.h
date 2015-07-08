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

// Backup and clear the MCUSR regester early in the AVR boot process (to
// ensure we don't enter a continual reset loop; see above comment).

// This funny blob of code should be included with almost every AVR program,
// even if they don't ever intentionally use the watchdog timer (WDT) at all.
// Modern Arduino bootloaders may include equivalent code and so not need
// this, but other programming methods (e.g. AVRISPmkII) should deninitely
// use it.  Simply add the mantra macro to the source file containing
// the main function, nothing more is required.
//
// The reason for this is that the WDT could just possibly get
// turned on accidently by bad code or even electrical noise,
// and if the MCUSR register isn't correctly cleared early in the
// boot process, an infinite series of resets might occur.  See
// http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html
// for details.
//
// As a bonus, this mantra gives you a global uint8_t variable called
// mcusr_mirror.  If the Arduino bootloader hasn't already cleared MCUSR
// when fetch_and_clear_mcusr() happens, this variable will contain the
// original contents of MCUSR, which can be used to determine the cause of
// the most recent reset (see the description of the MCUSR register in the
// AVR datasheet).  If the Arduino bootloader has already cleared MCUSR,
// then mcusr_mirror will contain nothing useful.  In practice this means
// that mcusr_mirror is only useful if you're not using the Arduino bootloader
// for programming, but instead are using an AVRISPmkII or the like (see the
// UPLOAD_METHOD definition in generic.mk).  Note that an extern declaration
// must be added to access this variable from other source files.
//
// Note that if the WDTON (watchdog timer always on) fust is programmed, the
// wdt_disable() call that this code makes will have no effect.  However,
// in this case the program will obviously need to be written to feed the
// watchdog anyway.  The mcusr_mirror should still be usable in this case.
//
#define WATCHDOG_TIMER_MCUSR_MANTRA                           \
                                                              \
  uint8_t mcusr_mirror __attribute__ ((section (".noinit"))); \
                                                              \
  void                                                        \
  fetch_and_clear_mcusr (void)                                \
    __attribute__((naked))                                    \
    __attribute__((section(".init3")));                       \
                                                              \
  void                                                        \
  fetch_and_clear_mcusr (void)                                \
  {                                                           \
    mcusr_mirror = MCUSR;                                     \
    MCUSR = 0x00;                                             \
    wdt_disable ();                                           \
  }

#define HIGH 0x01
#define LOW  0x00

// WARNING: of course some contexts might understand things differently.
// Fuse and lock bits read as zero when "programmed", for example.
#define TRUE  0x01
#define FALSE 0x00

// WARNING: unless the argument a contains an explicit cast to float or
// double, the results of CLOCK_CYCLES_TO_MICROSECONDS() are subject to
// integer truncation.  This is somewhat gross, but at least makes it clear
// when we can tolerate a float result.
#if F_CPU % 1000000UL != 0
// The time conversions below don't work right for F_CPU values that aren't a
// multiple of 1MHz.  If you don't think you care you might just remove this.
#  error F_CPU is not a multiple of 1MHz
#endif
// FIXME: its goofy to make this a functionesque macro (lose the parens)
#if F_CPU >= 1000000UL
#  define CLOCK_CYCLES_PER_MICROSECOND() (F_CPU / 1000000UL)
#  define CLOCK_CYCLES_TO_MICROSECONDS(a) \
     ((a) / CLOCK_CYCLES_PER_MICROSECOND())
#  define MICROSECONDS_TO_CLOCK_CYCLES(a) \
     ((a) * CLOCK_CYCLES_PER_MICROSECOND())
#else
#  error Interface untested with low-f clocks.  Remove #error and try :)
#  warning CLOCK_CYCLES_PER_MICROSECOND() would be less than 1 at this F_CPU
#  define CLOCK_CYCLES_TO_MICROSECONDS(a) (((a) * 1000UL) / (F_CPU / 1000UL))
#  define MICROSECONDS_TO_CLOCK_CYCLES(a) (((a) * (F_CPU / 1000UL)) / 1000UL)
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
// DDB5, PORTB, and PORB5 arguments in the right hand side of the original
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
// with GET_LASSERT_MESSAGE(), or cleared with CLEAR_LASSERT_MESSAGE().
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

// Macros for more readable/writable eight bit binary bit patterns {{{1
#define B0        UINT8_C (0)
#define B00       UINT8_C (0)
#define B000      UINT8_C (0)
#define B0000     UINT8_C (0)
#define B00000    UINT8_C (0)
#define B000000   UINT8_C (0)
#define B0000000  UINT8_C (0)
#define B00000000 UINT8_C (0)
#define B1        UINT8_C (1)
#define B01       UINT8_C (1)
#define B001      UINT8_C (1)
#define B0001     UINT8_C (1)
#define B00001    UINT8_C (1)
#define B000001   UINT8_C (1)
#define B0000001  UINT8_C (1)
#define B00000001 UINT8_C (1)
#define B10       UINT8_C (2)
#define B010      UINT8_C (2)
#define B0010     UINT8_C (2)
#define B00010    UINT8_C (2)
#define B000010   UINT8_C (2)
#define B0000010  UINT8_C (2)
#define B00000010 UINT8_C (2)
#define B11       UINT8_C (3)
#define B011      UINT8_C (3)
#define B0011     UINT8_C (3)
#define B00011    UINT8_C (3)
#define B000011   UINT8_C (3)
#define B0000011  UINT8_C (3)
#define B00000011 UINT8_C (3)
#define B100      UINT8_C (4)
#define B0100     UINT8_C (4)
#define B00100    UINT8_C (4)
#define B000100   UINT8_C (4)
#define B0000100  UINT8_C (4)
#define B00000100 UINT8_C (4)
#define B101      UINT8_C (5)
#define B0101     UINT8_C (5)
#define B00101    UINT8_C (5)
#define B000101   UINT8_C (5)
#define B0000101  UINT8_C (5)
#define B00000101 UINT8_C (5)
#define B110      UINT8_C (6)
#define B0110     UINT8_C (6)
#define B00110    UINT8_C (6)
#define B000110   UINT8_C (6)
#define B0000110  UINT8_C (6)
#define B00000110 UINT8_C (6)
#define B111      UINT8_C (7)
#define B0111     UINT8_C (7)
#define B00111    UINT8_C (7)
#define B000111   UINT8_C (7)
#define B0000111  UINT8_C (7)
#define B00000111 UINT8_C (7)
#define B1000     UINT8_C (8)
#define B01000    UINT8_C (8)
#define B001000   UINT8_C (8)
#define B0001000  UINT8_C (8)
#define B00001000 UINT8_C (8)
#define B1001     UINT8_C (9)
#define B01001    UINT8_C (9)
#define B001001   UINT8_C (9)
#define B0001001  UINT8_C (9)
#define B00001001 UINT8_C (9)
#define B1010     UINT8_C (10)
#define B01010    UINT8_C (10)
#define B001010   UINT8_C (10)
#define B0001010  UINT8_C (10)
#define B00001010 UINT8_C (10)
#define B1011     UINT8_C (11)
#define B01011    UINT8_C (11)
#define B001011   UINT8_C (11)
#define B0001011  UINT8_C (11)
#define B00001011 UINT8_C (11)
#define B1100     UINT8_C (12)
#define B01100    UINT8_C (12)
#define B001100   UINT8_C (12)
#define B0001100  UINT8_C (12)
#define B00001100 UINT8_C (12)
#define B1101     UINT8_C (13)
#define B01101    UINT8_C (13)
#define B001101   UINT8_C (13)
#define B0001101  UINT8_C (13)
#define B00001101 UINT8_C (13)
#define B1110     UINT8_C (14)
#define B01110    UINT8_C (14)
#define B001110   UINT8_C (14)
#define B0001110  UINT8_C (14)
#define B00001110 UINT8_C (14)
#define B1111     UINT8_C (15)
#define B01111    UINT8_C (15)
#define B001111   UINT8_C (15)
#define B0001111  UINT8_C (15)
#define B00001111 UINT8_C (15)
#define B10000    UINT8_C (16)
#define B010000   UINT8_C (16)
#define B0010000  UINT8_C (16)
#define B00010000 UINT8_C (16)
#define B10001    UINT8_C (17)
#define B010001   UINT8_C (17)
#define B0010001  UINT8_C (17)
#define B00010001 UINT8_C (17)
#define B10010    UINT8_C (18)
#define B010010   UINT8_C (18)
#define B0010010  UINT8_C (18)
#define B00010010 UINT8_C (18)
#define B10011    UINT8_C (19)
#define B010011   UINT8_C (19)
#define B0010011  UINT8_C (19)
#define B00010011 UINT8_C (19)
#define B10100    UINT8_C (20)
#define B010100   UINT8_C (20)
#define B0010100  UINT8_C (20)
#define B00010100 UINT8_C (20)
#define B10101    UINT8_C (21)
#define B010101   UINT8_C (21)
#define B0010101  UINT8_C (21)
#define B00010101 UINT8_C (21)
#define B10110    UINT8_C (22)
#define B010110   UINT8_C (22)
#define B0010110  UINT8_C (22)
#define B00010110 UINT8_C (22)
#define B10111    UINT8_C (23)
#define B010111   UINT8_C (23)
#define B0010111  UINT8_C (23)
#define B00010111 UINT8_C (23)
#define B11000    UINT8_C (24)
#define B011000   UINT8_C (24)
#define B0011000  UINT8_C (24)
#define B00011000 UINT8_C (24)
#define B11001    UINT8_C (25)
#define B011001   UINT8_C (25)
#define B0011001  UINT8_C (25)
#define B00011001 UINT8_C (25)
#define B11010    UINT8_C (26)
#define B011010   UINT8_C (26)
#define B0011010  UINT8_C (26)
#define B00011010 UINT8_C (26)
#define B11011    UINT8_C (27)
#define B011011   UINT8_C (27)
#define B0011011  UINT8_C (27)
#define B00011011 UINT8_C (27)
#define B11100    UINT8_C (28)
#define B011100   UINT8_C (28)
#define B0011100  UINT8_C (28)
#define B00011100 UINT8_C (28)
#define B11101    UINT8_C (29)
#define B011101   UINT8_C (29)
#define B0011101  UINT8_C (29)
#define B00011101 UINT8_C (29)
#define B11110    UINT8_C (30)
#define B011110   UINT8_C (30)
#define B0011110  UINT8_C (30)
#define B00011110 UINT8_C (30)
#define B11111    UINT8_C (31)
#define B011111   UINT8_C (31)
#define B0011111  UINT8_C (31)
#define B00011111 UINT8_C (31)
#define B100000   UINT8_C (32)
#define B0100000  UINT8_C (32)
#define B00100000 UINT8_C (32)
#define B100001   UINT8_C (33)
#define B0100001  UINT8_C (33)
#define B00100001 UINT8_C (33)
#define B100010   UINT8_C (34)
#define B0100010  UINT8_C (34)
#define B00100010 UINT8_C (34)
#define B100011   UINT8_C (35)
#define B0100011  UINT8_C (35)
#define B00100011 UINT8_C (35)
#define B100100   UINT8_C (36)
#define B0100100  UINT8_C (36)
#define B00100100 UINT8_C (36)
#define B100101   UINT8_C (37)
#define B0100101  UINT8_C (37)
#define B00100101 UINT8_C (37)
#define B100110   UINT8_C (38)
#define B0100110  UINT8_C (38)
#define B00100110 UINT8_C (38)
#define B100111   UINT8_C (39)
#define B0100111  UINT8_C (39)
#define B00100111 UINT8_C (39)
#define B101000   UINT8_C (40)
#define B0101000  UINT8_C (40)
#define B00101000 UINT8_C (40)
#define B101001   UINT8_C (41)
#define B0101001  UINT8_C (41)
#define B00101001 UINT8_C (41)
#define B101010   UINT8_C (42)
#define B0101010  UINT8_C (42)
#define B00101010 UINT8_C (42)
#define B101011   UINT8_C (43)
#define B0101011  UINT8_C (43)
#define B00101011 UINT8_C (43)
#define B101100   UINT8_C (44)
#define B0101100  UINT8_C (44)
#define B00101100 UINT8_C (44)
#define B101101   UINT8_C (45)
#define B0101101  UINT8_C (45)
#define B00101101 UINT8_C (45)
#define B101110   UINT8_C (46)
#define B0101110  UINT8_C (46)
#define B00101110 UINT8_C (46)
#define B101111   UINT8_C (47)
#define B0101111  UINT8_C (47)
#define B00101111 UINT8_C (47)
#define B110000   UINT8_C (48)
#define B0110000  UINT8_C (48)
#define B00110000 UINT8_C (48)
#define B110001   UINT8_C (49)
#define B0110001  UINT8_C (49)
#define B00110001 UINT8_C (49)
#define B110010   UINT8_C (50)
#define B0110010  UINT8_C (50)
#define B00110010 UINT8_C (50)
#define B110011   UINT8_C (51)
#define B0110011  UINT8_C (51)
#define B00110011 UINT8_C (51)
#define B110100   UINT8_C (52)
#define B0110100  UINT8_C (52)
#define B00110100 UINT8_C (52)
#define B110101   UINT8_C (53)
#define B0110101  UINT8_C (53)
#define B00110101 UINT8_C (53)
#define B110110   UINT8_C (54)
#define B0110110  UINT8_C (54)
#define B00110110 UINT8_C (54)
#define B110111   UINT8_C (55)
#define B0110111  UINT8_C (55)
#define B00110111 UINT8_C (55)
#define B111000   UINT8_C (56)
#define B0111000  UINT8_C (56)
#define B00111000 UINT8_C (56)
#define B111001   UINT8_C (57)
#define B0111001  UINT8_C (57)
#define B00111001 UINT8_C (57)
#define B111010   UINT8_C (58)
#define B0111010  UINT8_C (58)
#define B00111010 UINT8_C (58)
#define B111011   UINT8_C (59)
#define B0111011  UINT8_C (59)
#define B00111011 UINT8_C (59)
#define B111100   UINT8_C (60)
#define B0111100  UINT8_C (60)
#define B00111100 UINT8_C (60)
#define B111101   UINT8_C (61)
#define B0111101  UINT8_C (61)
#define B00111101 UINT8_C (61)
#define B111110   UINT8_C (62)
#define B0111110  UINT8_C (62)
#define B00111110 UINT8_C (62)
#define B111111   UINT8_C (63)
#define B0111111  UINT8_C (63)
#define B00111111 UINT8_C (63)
#define B1000000  UINT8_C (64)
#define B01000000 UINT8_C (64)
#define B1000001  UINT8_C (65)
#define B01000001 UINT8_C (65)
#define B1000010  UINT8_C (66)
#define B01000010 UINT8_C (66)
#define B1000011  UINT8_C (67)
#define B01000011 UINT8_C (67)
#define B1000100  UINT8_C (68)
#define B01000100 UINT8_C (68)
#define B1000101  UINT8_C (69)
#define B01000101 UINT8_C (69)
#define B1000110  UINT8_C (70)
#define B01000110 UINT8_C (70)
#define B1000111  UINT8_C (71)
#define B01000111 UINT8_C (71)
#define B1001000  UINT8_C (72)
#define B01001000 UINT8_C (72)
#define B1001001  UINT8_C (73)
#define B01001001 UINT8_C (73)
#define B1001010  UINT8_C (74)
#define B01001010 UINT8_C (74)
#define B1001011  UINT8_C (75)
#define B01001011 UINT8_C (75)
#define B1001100  UINT8_C (76)
#define B01001100 UINT8_C (76)
#define B1001101  UINT8_C (77)
#define B01001101 UINT8_C (77)
#define B1001110  UINT8_C (78)
#define B01001110 UINT8_C (78)
#define B1001111  UINT8_C (79)
#define B01001111 UINT8_C (79)
#define B1010000  UINT8_C (80)
#define B01010000 UINT8_C (80)
#define B1010001  UINT8_C (81)
#define B01010001 UINT8_C (81)
#define B1010010  UINT8_C (82)
#define B01010010 UINT8_C (82)
#define B1010011  UINT8_C (83)
#define B01010011 UINT8_C (83)
#define B1010100  UINT8_C (84)
#define B01010100 UINT8_C (84)
#define B1010101  UINT8_C (85)
#define B01010101 UINT8_C (85)
#define B1010110  UINT8_C (86)
#define B01010110 UINT8_C (86)
#define B1010111  UINT8_C (87)
#define B01010111 UINT8_C (87)
#define B1011000  UINT8_C (88)
#define B01011000 UINT8_C (88)
#define B1011001  UINT8_C (89)
#define B01011001 UINT8_C (89)
#define B1011010  UINT8_C (90)
#define B01011010 UINT8_C (90)
#define B1011011  UINT8_C (91)
#define B01011011 UINT8_C (91)
#define B1011100  UINT8_C (92)
#define B01011100 UINT8_C (92)
#define B1011101  UINT8_C (93)
#define B01011101 UINT8_C (93)
#define B1011110  UINT8_C (94)
#define B01011110 UINT8_C (94)
#define B1011111  UINT8_C (95)
#define B01011111 UINT8_C (95)
#define B1100000  UINT8_C (96)
#define B01100000 UINT8_C (96)
#define B1100001  UINT8_C (97)
#define B01100001 UINT8_C (97)
#define B1100010  UINT8_C (98)
#define B01100010 UINT8_C (98)
#define B1100011  UINT8_C (99)
#define B01100011 UINT8_C (99)
#define B1100100  UINT8_C (100)
#define B01100100 UINT8_C (100)
#define B1100101  UINT8_C (101)
#define B01100101 UINT8_C (101)
#define B1100110  UINT8_C (102)
#define B01100110 UINT8_C (102)
#define B1100111  UINT8_C (103)
#define B01100111 UINT8_C (103)
#define B1101000  UINT8_C (104)
#define B01101000 UINT8_C (104)
#define B1101001  UINT8_C (105)
#define B01101001 UINT8_C (105)
#define B1101010  UINT8_C (106)
#define B01101010 UINT8_C (106)
#define B1101011  UINT8_C (107)
#define B01101011 UINT8_C (107)
#define B1101100  UINT8_C (108)
#define B01101100 UINT8_C (108)
#define B1101101  UINT8_C (109)
#define B01101101 UINT8_C (109)
#define B1101110  UINT8_C (110)
#define B01101110 UINT8_C (110)
#define B1101111  UINT8_C (111)
#define B01101111 UINT8_C (111)
#define B1110000  UINT8_C (112)
#define B01110000 UINT8_C (112)
#define B1110001  UINT8_C (113)
#define B01110001 UINT8_C (113)
#define B1110010  UINT8_C (114)
#define B01110010 UINT8_C (114)
#define B1110011  UINT8_C (115)
#define B01110011 UINT8_C (115)
#define B1110100  UINT8_C (116)
#define B01110100 UINT8_C (116)
#define B1110101  UINT8_C (117)
#define B01110101 UINT8_C (117)
#define B1110110  UINT8_C (118)
#define B01110110 UINT8_C (118)
#define B1110111  UINT8_C (119)
#define B01110111 UINT8_C (119)
#define B1111000  UINT8_C (120)
#define B01111000 UINT8_C (120)
#define B1111001  UINT8_C (121)
#define B01111001 UINT8_C (121)
#define B1111010  UINT8_C (122)
#define B01111010 UINT8_C (122)
#define B1111011  UINT8_C (123)
#define B01111011 UINT8_C (123)
#define B1111100  UINT8_C (124)
#define B01111100 UINT8_C (124)
#define B1111101  UINT8_C (125)
#define B01111101 UINT8_C (125)
#define B1111110  UINT8_C (126)
#define B01111110 UINT8_C (126)
#define B1111111  UINT8_C (127)
#define B01111111 UINT8_C (127)
#define B10000000 UINT8_C (128)
#define B10000001 UINT8_C (129)
#define B10000010 UINT8_C (130)
#define B10000011 UINT8_C (131)
#define B10000100 UINT8_C (132)
#define B10000101 UINT8_C (133)
#define B10000110 UINT8_C (134)
#define B10000111 UINT8_C (135)
#define B10001000 UINT8_C (136)
#define B10001001 UINT8_C (137)
#define B10001010 UINT8_C (138)
#define B10001011 UINT8_C (139)
#define B10001100 UINT8_C (140)
#define B10001101 UINT8_C (141)
#define B10001110 UINT8_C (142)
#define B10001111 UINT8_C (143)
#define B10010000 UINT8_C (144)
#define B10010001 UINT8_C (145)
#define B10010010 UINT8_C (146)
#define B10010011 UINT8_C (147)
#define B10010100 UINT8_C (148)
#define B10010101 UINT8_C (149)
#define B10010110 UINT8_C (150)
#define B10010111 UINT8_C (151)
#define B10011000 UINT8_C (152)
#define B10011001 UINT8_C (153)
#define B10011010 UINT8_C (154)
#define B10011011 UINT8_C (155)
#define B10011100 UINT8_C (156)
#define B10011101 UINT8_C (157)
#define B10011110 UINT8_C (158)
#define B10011111 UINT8_C (159)
#define B10100000 UINT8_C (160)
#define B10100001 UINT8_C (161)
#define B10100010 UINT8_C (162)
#define B10100011 UINT8_C (163)
#define B10100100 UINT8_C (164)
#define B10100101 UINT8_C (165)
#define B10100110 UINT8_C (166)
#define B10100111 UINT8_C (167)
#define B10101000 UINT8_C (168)
#define B10101001 UINT8_C (169)
#define B10101010 UINT8_C (170)
#define B10101011 UINT8_C (171)
#define B10101100 UINT8_C (172)
#define B10101101 UINT8_C (173)
#define B10101110 UINT8_C (174)
#define B10101111 UINT8_C (175)
#define B10110000 UINT8_C (176)
#define B10110001 UINT8_C (177)
#define B10110010 UINT8_C (178)
#define B10110011 UINT8_C (179)
#define B10110100 UINT8_C (180)
#define B10110101 UINT8_C (181)
#define B10110110 UINT8_C (182)
#define B10110111 UINT8_C (183)
#define B10111000 UINT8_C (184)
#define B10111001 UINT8_C (185)
#define B10111010 UINT8_C (186)
#define B10111011 UINT8_C (187)
#define B10111100 UINT8_C (188)
#define B10111101 UINT8_C (189)
#define B10111110 UINT8_C (190)
#define B10111111 UINT8_C (191)
#define B11000000 UINT8_C (192)
#define B11000001 UINT8_C (193)
#define B11000010 UINT8_C (194)
#define B11000011 UINT8_C (195)
#define B11000100 UINT8_C (196)
#define B11000101 UINT8_C (197)
#define B11000110 UINT8_C (198)
#define B11000111 UINT8_C (199)
#define B11001000 UINT8_C (200)
#define B11001001 UINT8_C (201)
#define B11001010 UINT8_C (202)
#define B11001011 UINT8_C (203)
#define B11001100 UINT8_C (204)
#define B11001101 UINT8_C (205)
#define B11001110 UINT8_C (206)
#define B11001111 UINT8_C (207)
#define B11010000 UINT8_C (208)
#define B11010001 UINT8_C (209)
#define B11010010 UINT8_C (210)
#define B11010011 UINT8_C (211)
#define B11010100 UINT8_C (212)
#define B11010101 UINT8_C (213)
#define B11010110 UINT8_C (214)
#define B11010111 UINT8_C (215)
#define B11011000 UINT8_C (216)
#define B11011001 UINT8_C (217)
#define B11011010 UINT8_C (218)
#define B11011011 UINT8_C (219)
#define B11011100 UINT8_C (220)
#define B11011101 UINT8_C (221)
#define B11011110 UINT8_C (222)
#define B11011111 UINT8_C (223)
#define B11100000 UINT8_C (224)
#define B11100001 UINT8_C (225)
#define B11100010 UINT8_C (226)
#define B11100011 UINT8_C (227)
#define B11100100 UINT8_C (228)
#define B11100101 UINT8_C (229)
#define B11100110 UINT8_C (230)
#define B11100111 UINT8_C (231)
#define B11101000 UINT8_C (232)
#define B11101001 UINT8_C (233)
#define B11101010 UINT8_C (234)
#define B11101011 UINT8_C (235)
#define B11101100 UINT8_C (236)
#define B11101101 UINT8_C (237)
#define B11101110 UINT8_C (238)
#define B11101111 UINT8_C (239)
#define B11110000 UINT8_C (240)
#define B11110001 UINT8_C (241)
#define B11110010 UINT8_C (242)
#define B11110011 UINT8_C (243)
#define B11110100 UINT8_C (244)
#define B11110101 UINT8_C (245)
#define B11110110 UINT8_C (246)
#define B11110111 UINT8_C (247)
#define B11111000 UINT8_C (248)
#define B11111001 UINT8_C (249)
#define B11111010 UINT8_C (250)
#define B11111011 UINT8_C (251)
#define B11111100 UINT8_C (252)
#define B11111101 UINT8_C (253)
#define B11111110 UINT8_C (254)
#define B11111111 UINT8_C (255)
// }}}1

#endif // UTIL_H
