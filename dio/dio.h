// Simple digital IO macros to hide some of the ugliness of port bit value
// manipulation.

// vim: foldmethod=marker

#ifndef DIO_H
#define DIO_H

#include <avr/io.h>
#include <inttypes.h>



///////////////////////////////////////////////////////////////////////////////
// 
// How This Interface Works
//
// This interface provides simple pin-at-a-time initialization, reading,
// and writing of the digital IO pins of the ATMega 328p.
//
// WARNING: this interface provides macros to control pins that are
// normally not available for use as general digital IO on an Arduino.
// See the below section 'Notes About Particular Pins'.
//
// WARNING: some of the pin configuration abstration provided by the
// initialization macros could cause trouble in some unusual situations.
// See the below section 'Pin Initialization Details'.
//
// Example of use:
//
//   DIO_INIT_PB0 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
//   DIO_INIT_PB1 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
//
//   uint8_t pb0_value = some_function_returing_boolean ();
//   DIO_SET_PB0 (pb0_value);
//   uint8_t pb1_value = DIO_READ_PB1 ();
//
// When setting a pin to a value known at compile time, its a tiny bit faster
// (and smaller code) to set the value like this:
//
//   DIO_SET_PB0_HIGH ();
//
// I finds these macros easier to read and remember than the individual
// bit fiddling instructions and wait-till set code.
//
// This interface doesn't support configuring/reading/writing multiple pins
// that use the same port in a single instruction, which is possible with
// the raw memory map read/write interface provided by AVR Libc.  
//
// For those who like the alternate numbering system that Aruidno's use
// for their preferred digital IO pins, macros that work like this are
// also provided:
//
//   DIO_INIT_DIGITAL_8 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
//
// All caveats described for the underlying macros apply.


///////////////////////////////////////////////////////////////////////////////
// 
// Notes About Particular Pins
//
// All the IO pins on the ATMega chips can be set to perform alternate
// functions other than general digital IO.  The Arduino uses some pins
// for its own pre-set hardware or software purposes:
//
//   * PB3, PB4     used for in-system programming (ISP)
//
//   * PB5          connected to to ground via ~1 kohm resistor and LED,
//                  also used for in-system programming (ISP)
//
//   * PB6, PB7     used for the external oscillator
//
//   * PC6          used as the reset pin, and for in-system programming
//
//   * PD0, PD1     set up for serial communication by Arduino bootloader
//
// These pins cannot be used for arbitrary general digital IO in the normal
// Arduino configuration.  PB5 can be usefully set for output to control
// the onboard LED.
//
// For completeness and easy migration to simpler hardware, this interface
// provides macros for these pins.  At least the following considerations
// apply to their use:
//
//   * PB3 and PB4 can be used, although tying them firmly low or high
//     seems certain to interfere with in-system programming (ISP).  I don't
//     fully understand ISP/digital IO interaction.
//
//   * PB5 can be used, provided one doesn't mind it being pulled low and
//     driving the on-board LED as a side effect (note that activating the
//     internal pull-up resistor will result in a pointless current drain
//     and still fail to pull the input high).  Also, tying it firmly low
//     or high seem certain to interfere with in-system programming (ISP).
//
//   * PD0 and PD1 can be utilized as general digital IO when using
//     an in-system programmer (instead of serial programming via the
//     bootloader).  Note that the bootloader must be fully removed (which
//     ISP does) before this can work.
//
//   * PB6, PB7 and PC6 cannot be used for general digital IO without
//     hardware changes.


///////////////////////////////////////////////////////////////////////////////
// 
// Pin Initialization Details
//
// In general, it is possible to re-initialize a pin to change it from
// input to output or vice versa, or to enable or disable its internal
// pull-up resistor.  However, there is one issue that results from the
// order in which the initialiazation macros do things.
//
// When initializing a pin for input, the pin is first set for input,
// then the internal pullup resistor enabled if requested.  This means
// that the pin might float for a few microseconds (which might possibly
// result in a spurious pin change interrupt if those are enabled (and
// there is no external pull-up or pull-down resister).  We do things in
// this order to ensure that we don't have to risk a spurious change to the
// output value in case the pin is being reconfigured from output to input.
// When initializing a pin for output, the requested initial value is first
// set and the the pin direction set for output.  This might likewise result
// in a momentarily floating input pin (and potential interrupt).

// FIXME: possibly all the loop_until_bit_is_* calls could be replaced with
// single *hardware* no-ops.  I'm just haven't looked up how to produce
// them with for sure with C or GCC.

// I can't bring myself to drag in another utility header just for this junk.
#ifndef HIGH
#  define HIGH 0x1
#else
#  if HIGH != 1
#    error uh oh, HIGH != 1
#  endif
#endif
#ifndef LOW
#  define LOW 0x0
#else
#  if LOW != 0
#    error uh oh, LOW != 0
#  endif
#endif

// Pin Initialization {{{1

// These macros can be used to make DIO_INIT_* calls more readable.
#define DIO_INPUT  1
#define DIO_OUTPUT 0
#define DIO_ENABLE_PULLUP 1
#define DIO_DISABLE_PULLUP 1
#define DIO_DONT_CARE 0

// Pin PB* Initialization {{{2 

#define DIO_INIT_PB0(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB0)); \
        loop_until_bit_is_clear (DDRB, DDB0); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB0); \
          loop_until_bit_is_set (PORTB, PORTB0); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB0)); \
          loop_until_bit_is_clear (PORTB, PORTB0); \
        } \
      } \
      else { \
        DIO_SET_PB0 (initial_value); \
        DDRB |= _BV (DDB0); \
        loop_until_bit_is_set (DDRB, DDB0); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PB1(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB1)); \
        loop_until_bit_is_clear (DDRB, DDB1); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB1); \
          loop_until_bit_is_set (PORTB, PORTB1); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB1)); \
          loop_until_bit_is_clear (PORTB, PORTB1); \
        } \
      } \
      else { \
        DIO_SET_PB1 (initial_value); \
        DDRB |= _BV (DDB1); \
        loop_until_bit_is_set (DDRB, DDB1); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PB2(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB2)); \
        loop_until_bit_is_clear (DDRB, DDB2); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB2); \
          loop_until_bit_is_set (PORTB, PORTB2); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB2)); \
          loop_until_bit_is_clear (PORTB, PORTB2); \
        } \
      } \
      else { \
        DIO_SET_PB2 (initial_value); \
        DDRB |= _BV (DDB2); \
        loop_until_bit_is_set (DDRB, DDB2); \
      } \
      break; \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB3(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB3)); \
        loop_until_bit_is_clear (DDRB, DDB3); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB3); \
          loop_until_bit_is_set (PORTB, PORTB3); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB3)); \
          loop_until_bit_is_clear (PORTB, PORTB3); \
        } \
      } \
      else { \
        DIO_SET_PB3 (initial_value); \
        DDRB |= _BV (DDB3); \
        loop_until_bit_is_set (DDRB, DDB3); \
      } \
      break; \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB4(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB4)); \
        loop_until_bit_is_clear (DDRB, DDB4); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB4); \
          loop_until_bit_is_set (PORTB, PORTB4); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB4)); \
          loop_until_bit_is_clear (PORTB, PORTB4); \
        } \
      } \
      else { \
        DIO_SET_PB4 (initial_value); \
        DDRB |= _BV (DDB4); \
        loop_until_bit_is_set (DDRB, DDB4); \
      } \
      break; \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB5(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB5)); \
        loop_until_bit_is_clear (DDRB, DDB5); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB5); \
          loop_until_bit_is_set (PORTB, PORTB5); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB5)); \
          loop_until_bit_is_clear (PORTB, PORTB5); \
        } \
      } \
      else { \
        DIO_SET_PB5 (initial_value); \
        DDRB |= _BV (DDB5); \
        loop_until_bit_is_set (DDRB, DDB5); \
      } \
      break; \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB6(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB6)); \
        loop_until_bit_is_clear (DDRB, DDB6); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB6); \
          loop_until_bit_is_set (PORTB, PORTB6); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB6)); \
          loop_until_bit_is_clear (PORTB, PORTB6); \
        } \
      } \
      else { \
        DIO_SET_PB6 (initial_value); \
        DDRB |= _BV (DDB6); \
        loop_until_bit_is_set (DDRB, DDB6); \
      } \
      break; \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB7(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRB &= ~(_BV (DDB7)); \
        loop_until_bit_is_clear (DDRB, DDB7); \
        if ( enable_pullup ) { \
          PORTB |= _BV (PORTB7); \
          loop_until_bit_is_set (PORTB, PORTB7); \
        } \
        else { \
          PORTB &= ~(_BV (PORTB7)); \
          loop_until_bit_is_clear (PORTB, PORTB7); \
        } \
      } \
      else { \
        DIO_SET_PB7 (initial_value); \
        DDRB |= _BV (DDB7); \
        loop_until_bit_is_set (DDRB, DDB7); \
      } \
      break; \
  } while ( 0 )

/// }}}2

// Pin PC* Initialization {{{2

#define DIO_INIT_PC0(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRC &= ~(_BV (DDC0)); \
        loop_until_bit_is_clear (DDRC, DDC0); \
        if ( enable_pullup ) { \
          PORTC |= _BV (PORTC0); \
          loop_until_bit_is_set (PORTC, PORTC0); \
        } \
        else { \
          PORTC &= ~(_BV (PORTC0)); \
          loop_until_bit_is_clear (PORTC, PORTC0); \
        } \
      } \
      else { \
        DIO_SET_PC0 (initial_value); \
        DDRC |= _BV (DDC0); \
        loop_until_bit_is_set (DDRC, DDC0); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PC1(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRC &= ~(_BV (DDC1)); \
        loop_until_bit_is_clear (DDRC, DDC1); \
        if ( enable_pullup ) { \
          PORTC |= _BV (PORTC1); \
          loop_until_bit_is_set (PORTC, PORTC1); \
        } \
        else { \
          PORTC &= ~(_BV (PORTC1)); \
          loop_until_bit_is_clear (PORTC, PORTC1); \
        } \
      } \
      else { \
        DIO_SET_PC1 (initial_value); \
        DDRC |= _BV (DDC1); \
        loop_until_bit_is_set (DDRC, DDC1); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PC2(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRC &= ~(_BV (DDC2)); \
        loop_until_bit_is_clear (DDRC, DDC2); \
        if ( enable_pullup ) { \
          PORTC |= _BV (PORTC2); \
          loop_until_bit_is_set (PORTC, PORTC2); \
        } \
        else { \
          PORTC &= ~(_BV (PORTC2)); \
          loop_until_bit_is_clear (PORTC, PORTC2); \
        } \
      } \
      else { \
        DIO_SET_PC2 (initial_value); \
        DDRC |= _BV (DDC2); \
        loop_until_bit_is_set (DDRC, DDC2); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PC3(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRC &= ~(_BV (DDC3)); \
        loop_until_bit_is_clear (DDRC, DDC3); \
        if ( enable_pullup ) { \
          PORTC |= _BV (PORTC3); \
          loop_until_bit_is_set (PORTC, PORTC3); \
        } \
        else { \
          PORTC &= ~(_BV (PORTC3)); \
          loop_until_bit_is_clear (PORTC, PORTC3); \
        } \
      } \
      else { \
        DIO_SET_PC3 (initial_value); \
        DDRC |= _BV (DDC3); \
        loop_until_bit_is_set (DDRC, DDC3); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PC4(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRC &= ~(_BV (DDC4)); \
        loop_until_bit_is_clear (DDRC, DDC4); \
        if ( enable_pullup ) { \
          PORTC |= _BV (PORTC4); \
          loop_until_bit_is_set (PORTC, PORTC4); \
        } \
        else { \
          PORTC &= ~(_BV (PORTC4)); \
          loop_until_bit_is_clear (PORTC, PORTC4); \
        } \
      } \
      else { \
        DIO_SET_PC4 (initial_value); \
        DDRC |= _BV (DDC4); \
        loop_until_bit_is_set (DDRC, DDC4); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PC5(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRC &= ~(_BV (DDC5)); \
        loop_until_bit_is_clear (DDRC, DDC5); \
        if ( enable_pullup ) { \
          PORTC |= _BV (PORTC5); \
          loop_until_bit_is_set (PORTC, PORTC5); \
        } \
        else { \
          PORTC &= ~(_BV (PORTC5)); \
          loop_until_bit_is_clear (PORTC, PORTC5); \
        } \
      } \
      else { \
        DIO_SET_PC5 (initial_value); \
        DDRC |= _BV (DDC5); \
        loop_until_bit_is_set (DDRC, DDC5); \
      } \
      break; \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PC6(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRC &= ~(_BV (DDC6)); \
        loop_until_bit_is_clear (DDRC, DDC6); \
        if ( enable_pullup ) { \
          PORTC |= _BV (PORTC6); \
          loop_until_bit_is_set (PORTC, PORTC6); \
        } \
        else { \
          PORTC &= ~(_BV (PORTC6)); \
          loop_until_bit_is_clear (PORTC, PORTC6); \
        } \
      } \
      else { \
        DIO_SET_PC6 (initial_value); \
        DDRC |= _BV (DDC6); \
        loop_until_bit_is_set (DDRC, DDC6); \
      } \
      break; \
  } while ( 0 )

// }}}2

// Pin PD* Initialization {{{2

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PD0(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD0)); \
        loop_until_bit_is_clear (DDRD, DDD0); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD0); \
          loop_until_bit_is_set (PORTD, PORTD0); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD0)); \
          loop_until_bit_is_clear (PORTD, PORTD0); \
        } \
      } \
      else { \
        DIO_SET_PD0 (initial_value); \
        DDRD |= _BV (DDD0); \
        loop_until_bit_is_set (DDRD, DDD0); \
      } \
      break; \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PD1(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD1)); \
        loop_until_bit_is_clear (DDRD, DDD1); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD1); \
          loop_until_bit_is_set (PORTD, PORTD1); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD1)); \
          loop_until_bit_is_clear (PORTD, PORTD1); \
        } \
      } \
      else { \
        DIO_SET_PD1 (initial_value); \
        DDRD |= _BV (DDD1); \
        loop_until_bit_is_set (DDRD, DDD1); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PD2(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD2)); \
        loop_until_bit_is_clear (DDRD, DDD2); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD2); \
          loop_until_bit_is_set (PORTD, PORTD2); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD2)); \
          loop_until_bit_is_clear (PORTD, PORTD2); \
        } \
      } \
      else { \
        DIO_SET_PD2 (initial_value); \
        DDRD |= _BV (DDD2); \
        loop_until_bit_is_set (DDRD, DDD2); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PD3(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD3)); \
        loop_until_bit_is_clear (DDRD, DDD3); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD3); \
          loop_until_bit_is_set (PORTD, PORTD3); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD3)); \
          loop_until_bit_is_clear (PORTD, PORTD3); \
        } \
      } \
      else { \
        DIO_SET_PD3 (initial_value); \
        DDRD |= _BV (DDD3); \
        loop_until_bit_is_set (DDRD, DDD3); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PD4(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD4)); \
        loop_until_bit_is_clear (DDRD, DDD4); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD4); \
          loop_until_bit_is_set (PORTD, PORTD4); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD4)); \
          loop_until_bit_is_clear (PORTD, PORTD4); \
        } \
      } \
      else { \
        DIO_SET_PD4 (initial_value); \
        DDRD |= _BV (DDD4); \
        loop_until_bit_is_set (DDRD, DDD4); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PD5(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD5)); \
        loop_until_bit_is_clear (DDRD, DDD5); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD5); \
          loop_until_bit_is_set (PORTD, PORTD5); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD5)); \
          loop_until_bit_is_clear (PORTD, PORTD5); \
        } \
      } \
      else { \
        DIO_SET_PD5 (initial_value); \
        DDRD |= _BV (DDD5); \
        loop_until_bit_is_set (DDRD, DDD5); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PD6(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD6)); \
        loop_until_bit_is_clear (DDRD, DDD6); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD6); \
          loop_until_bit_is_set (PORTD, PORTD6); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD6)); \
          loop_until_bit_is_clear (PORTD, PORTD6); \
        } \
      } \
      else { \
        DIO_SET_PD6 (initial_value); \
        DDRD |= _BV (DDD6); \
        loop_until_bit_is_set (DDRD, DDD6); \
      } \
      break; \
  } while ( 0 )

#define DIO_INIT_PD7(for_input, enable_pullup, initial_value) \
  do { \
      if ( for_input ) { \
        DDRD &= ~(_BV (DDD7)); \
        loop_until_bit_is_clear (DDRD, DDD7); \
        if ( enable_pullup ) { \
          PORTD |= _BV (PORTD7); \
          loop_until_bit_is_set (PORTD, PORTD7); \
        } \
        else { \
          PORTD &= ~(_BV (PORTD7)); \
          loop_until_bit_is_clear (PORTD, PORTD7); \
        } \
      } \
      else { \
        DIO_SET_PD7 (initial_value); \
        DDRD |= _BV (DDD7); \
        loop_until_bit_is_set (DDRD, DDD7); \
      } \
      break; \
  } while ( 0 )

// }}}2

// }}}1

// Pin Setting {{{1

// Pins PB* Without Argument {{{2
#define DIO_SET_PB0_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB0)); \
    loop_until_bit_is_clear (PORTB, PORTB0); \
  } while ( 0 )

#define DIO_SET_PB0_HIGH() \
  do { \
    PORTB |= _BV (PORTB0); \
    loop_until_bit_is_set (PORTB, PORTB0); \
  } while ( 0 )


#define DIO_SET_PB1_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB1)); \
    loop_until_bit_is_clear (PORTB, PORTB1); \
  } while ( 0 )

#define DIO_SET_PB1_HIGH() \
  do { \
    PORTB |= _BV (PORTB1); \
    loop_until_bit_is_set (PORTB, PORTB1); \
  } while ( 0 )


#define DIO_SET_PB2_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB2)); \
    loop_until_bit_is_clear (PORTB, PORTB2); \
  } while ( 0 )

#define DIO_SET_PB2_HIGH() \
  do { \
    PORTB |= _BV (PORTB2); \
    loop_until_bit_is_set (PORTB, PORTB2); \
  } while ( 0 )


// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB3_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB3)); \
    loop_until_bit_is_clear (PORTB, PORTB3); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB3_HIGH() \
  do { \
    PORTB |= _BV (PORTB3); \
    loop_until_bit_is_set (PORTB, PORTB3); \
  } while ( 0 )


// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB4_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB4)); \
    loop_until_bit_is_clear (PORTB, PORTB4); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB4_HIGH() \
  do { \
    PORTB |= _BV (PORTB4); \
    loop_until_bit_is_set (PORTB, PORTB4); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB5_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB5)); \
    loop_until_bit_is_clear (PORTB, PORTB5); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB5_HIGH() \
  do { \
    PORTB |= _BV (PORTB5); \
    loop_until_bit_is_set (PORTB, PORTB5); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB6_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB6)); \
    loop_until_bit_is_clear (PORTB, PORTB6); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB6_HIGH() \
  do { \
    PORTB |= _BV (PORTB6); \
    loop_until_bit_is_set (PORTB, PORTB6); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB7_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB7)); \
    loop_until_bit_is_clear (PORTB, PORTB7); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB7_HIGH() \
  do { \
    PORTB |= _BV (PORTB7); \
    loop_until_bit_is_set (PORTB, PORTB7); \
  } while ( 0 )

// }}}2

// Pins PC* Without Argument {{{2

#define DIO_SET_PC0_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC0)); \
    loop_until_bit_is_clear (PORTC, PORTC0); \
  } while ( 0 )

#define DIO_SET_PC0_HIGH() \
  do { \
    PORTC |= _BV (PORTC0); \
    loop_until_bit_is_set (PORTC, PORTC0); \
  } while ( 0 )


#define DIO_SET_PC1_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC1)); \
    loop_until_bit_is_clear (PORTC, PORTC1); \
  } while ( 0 )

#define DIO_SET_PC1_HIGH() \
  do { \
    PORTC |= _BV (PORTC1); \
    loop_until_bit_is_set (PORTC, PORTC1); \
  } while ( 0 )


#define DIO_SET_PC2_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC2)); \
    loop_until_bit_is_clear (PORTC, PORTC2); \
  } while ( 0 )

#define DIO_SET_PC2_HIGH() \
  do { \
    PORTC |= _BV (PORTC2); \
    loop_until_bit_is_set (PORTC, PORTC2); \
  } while ( 0 )


#define DIO_SET_PC3_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC3)); \
    loop_until_bit_is_clear (PORTC, PORTC3); \
  } while ( 0 )

#define DIO_SET_PC3_HIGH() \
  do { \
    PORTC |= _BV (PORTC3); \
    loop_until_bit_is_set (PORTC, PORTC3); \
  } while ( 0 )


#define DIO_SET_PC4_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC4)); \
    loop_until_bit_is_clear (PORTC, PORTC4); \
  } while ( 0 )

#define DIO_SET_PC4_HIGH() \
  do { \
    PORTC |= _BV (PORTC4); \
    loop_until_bit_is_set (PORTC, PORTC4); \
  } while ( 0 )


#define DIO_SET_PC5_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC5)); \
    loop_until_bit_is_clear (PORTC, PORTC5); \
  } while ( 0 )

#define DIO_SET_PC5_HIGH() \
  do { \
    PORTC |= _BV (PORTC5); \
    loop_until_bit_is_set (PORTC, PORTC5); \
  } while ( 0 )


// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PC6_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC6)); \
    loop_until_bit_is_clear (PORTC, PORTC6); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PC6_HIGH() \
  do { \
    PORTC |= _BV (PORTC6); \
    loop_until_bit_is_set (PORTC, PORTC6); \
  } while ( 0 )

// }}}2

// Pins PD* Without Argument {{{2

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD0_HIGH() \
  do { \
    PORTD |= _BV (PORTD0); \
    loop_until_bit_is_set (PORTD, PORTD0); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD0_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD0)); \
    loop_until_bit_is_clear (PORTD, PORTD0); \
  } while ( 0 )


// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD1_HIGH() \
  do { \
    PORTD |= _BV (PORTD1); \
    loop_until_bit_is_set (PORTD, PORTD1); \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD1_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD1)); \
    loop_until_bit_is_clear (PORTD, PORTD1); \
  } while ( 0 )


#define DIO_SET_PD2_HIGH() \
  do { \
    PORTD |= _BV (PORTD2); \
    loop_until_bit_is_set (PORTD, PORTD2); \
  } while ( 0 )

#define DIO_SET_PD2_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD2)); \
    loop_until_bit_is_clear (PORTD, PORTD2); \
  } while ( 0 )


#define DIO_SET_PD3_HIGH() \
  do { \
    PORTD |= _BV (PORTD3); \
    loop_until_bit_is_set (PORTD, PORTD3); \
  } while ( 0 )

#define DIO_SET_PD3_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD3)); \
    loop_until_bit_is_clear (PORTD, PORTD3); \
  } while ( 0 )


#define DIO_SET_PD4_HIGH() \
  do { \
    PORTD |= _BV (PORTD4); \
    loop_until_bit_is_set (PORTD, PORTD4); \
  } while ( 0 )

#define DIO_SET_PD4_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD4)); \
    loop_until_bit_is_clear (PORTD, PORTD4); \
  } while ( 0 )


#define DIO_SET_PD5_HIGH() \
  do { \
    PORTD |= _BV (PORTD5); \
    loop_until_bit_is_set (PORTD, PORTD5); \
  } while ( 0 )

#define DIO_SET_PD5_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD5)); \
    loop_until_bit_is_clear (PORTD, PORTD5); \
  } while ( 0 )


#define DIO_SET_PD6_HIGH() \
  do { \
    PORTD |= _BV (PORTD6); \
    loop_until_bit_is_set (PORTD, PORTD6); \
  } while ( 0 )

#define DIO_SET_PD6_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD6)); \
    loop_until_bit_is_clear (PORTD, PORTD6); \
  } while ( 0 )


#define DIO_SET_PD7_HIGH() \
  do { \
    PORTD |= _BV (PORTD7); \
    loop_until_bit_is_set (PORTD, PORTD7); \
  } while ( 0 )

#define DIO_SET_PD7_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD7)); \
    loop_until_bit_is_clear (PORTD, PORTD7); \
  } while ( 0 )

// }}}2

// Pins PB* With Argument {{{2

#define DIO_SET_PB0(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB0_HIGH (); \
    } \
    else { \
      DIO_SET_PB0_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PB1(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB1_HIGH (); \
    } \
    else { \
      DIO_SET_PB1_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PB2(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB2_HIGH (); \
    } \
    else { \
      DIO_SET_PB2_LOW (); \
    } \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB3(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB3_HIGH (); \
    } \
    else { \
      DIO_SET_PB3_LOW (); \
    } \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB4(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB4_HIGH (); \
    } \
    else { \
      DIO_SET_PB4_LOW (); \
    } \
  } while ( 0 )
 
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB5(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB5_HIGH (); \
    } \
    else { \
      DIO_SET_PB5_LOW (); \
    } \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB6(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB6_HIGH (); \
    } \
    else { \
      DIO_SET_PB6_LOW (); \
    } \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB7(value) \
  do { \
    if ( value ) { \
      DIO_SET_PB7_HIGH (); \
    } \
    else { \
      DIO_SET_PB7_LOW (); \
    } \
  } while ( 0 )

// }}}2

// Pins PC* With Argument {{{2

#define DIO_SET_PC0(value) \
  do { \
    if ( value ) { \
      DIO_SET_PC0_HIGH (); \
    } \
    else { \
      DIO_SET_PC0_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PC1(value) \
  do { \
    if ( value ) { \
      DIO_SET_PC1_HIGH (); \
    } \
    else { \
      DIO_SET_PC1_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PC2(value) \
  do { \
    if ( value ) { \
      DIO_SET_PC2_HIGH (); \
    } \
    else { \
      DIO_SET_PC2_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PC3(value) \
  do { \
    if ( value ) { \
      DIO_SET_PC3_HIGH (); \
    } \
    else { \
      DIO_SET_PC3_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PC4(value) \
  do { \
    if ( value ) { \
      DIO_SET_PC4_HIGH (); \
    } \
    else { \
      DIO_SET_PC4_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PC5(value) \
  do { \
    if ( value ) { \
      DIO_SET_PC5_HIGH (); \
    } \
    else { \
      DIO_SET_PC5_LOW (); \
    } \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PC6(value) \
  do { \
    if ( value ) { \
      DIO_SET_PC6_HIGH (); \
    } \
    else { \
      DIO_SET_PC6_LOW (); \
    } \
  } while ( 0 )

// }}}2

// Pins PD* With Argument {{{2

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD0(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD0_HIGH (); \
    } \
    else { \
      DIO_SET_PD0_LOW (); \
    } \
  } while ( 0 )

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD1(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD1_HIGH (); \
    } \
    else { \
      DIO_SET_PD1_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PD2(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD2_HIGH (); \
    } \
    else { \
      DIO_SET_PD2_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PD3(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD3_HIGH (); \
    } \
    else { \
      DIO_SET_PD3_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PD4(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD4_HIGH (); \
    } \
    else { \
      DIO_SET_PD4_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PD5(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD5_HIGH (); \
    } \
    else { \
      DIO_SET_PD5_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PD6(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD6_HIGH (); \
    } \
    else { \
      DIO_SET_PD6_LOW (); \
    } \
  } while ( 0 )

#define DIO_SET_PD7(value) \
  do { \
    if ( value ) { \
      DIO_SET_PD7_HIGH (); \
    } \
    else { \
      DIO_SET_PD7_LOW (); \
    } \
  } while ( 0 )

// }}}2

// }}}1

// Pin Reading {{{1

#define DIO_READ_PB0() (PINB & _BV (PINB0))
#define DIO_READ_PB1() (PINB & _BV (PINB1))
#define DIO_READ_PB2() (PINB & _BV (PINB2))
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB3() (PINB & _BV (PINB3))
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB4() (PINB & _BV (PINB4))
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB5() (PINB & _BV (PINB5))
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB6() (PINB & _BV (PINB6))
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB7() (PINB & _BV (PINB7))

#define DIO_READ_PC0() (PINC & _BV (PINC0))
#define DIO_READ_PC1() (PINC & _BV (PINC1))
#define DIO_READ_PC2() (PINC & _BV (PINC2))
#define DIO_READ_PC3() (PINC & _BV (PINC3))
#define DIO_READ_PC4() (PINC & _BV (PINC4))
#define DIO_READ_PC5() (PINC & _BV (PINC5))
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PC6() (PINC & _BV (PINC6))

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PD0() (PIND & _BV (PIND0))
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PD1() (PIND & _BV (PIND1))
#define DIO_READ_PD2() (PIND & _BV (PIND2))
#define DIO_READ_PD3() (PIND & _BV (PIND3))
#define DIO_READ_PD4() (PIND & _BV (PIND4))
#define DIO_READ_PD5() (PIND & _BV (PIND5))
#define DIO_READ_PD6() (PIND & _BV (PIND6))
#define DIO_READ_PD7() (PIND & _BV (PIND7))

// }}}1

// Arduino Compatible Pin Number Macros {{{1

// WARNING: all caveats described for the underlying macros apply.

#define DIO_INIT_DIGITAL_0 DIO_INIT_PD0
#define DIO_INIT_DIGITAL_1 DIO_INIT_PD1
#define DIO_INIT_DIGITAL_2 DIO_INIT_PD2
#define DIO_INIT_DIGITAL_3 DIO_INIT_PD3
#define DIO_INIT_DIGITAL_4 DIO_INIT_PD4
#define DIO_INIT_DIGITAL_5 DIO_INIT_PD5
#define DIO_INIT_DIGITAL_6 DIO_INIT_PD6
#define DIO_INIT_DIGITAL_7 DIO_INIT_PD7
#define DIO_INIT_DIGITAL_8 DIO_INIT_PB0
#define DIO_INIT_DIGITAL_9 DIO_INIT_PB1
#define DIO_INIT_DIGITAL_10 DIO_INIT_PB2
#define DIO_INIT_DIGITAL_11 DIO_INIT_PB3
#define DIO_INIT_DIGITAL_12 DIO_INIT_PB4
#define DIO_INIT_DIGITAL_13 DIO_INIT_PB5

#define DIO_SET_DIGITAL_0_LOW DIO_SET_PD0_LOW
#define DIO_SET_DIGITAL_1_LOW DIO_SET_PD1_LOW
#define DIO_SET_DIGITAL_2_LOW DIO_SET_PD2_LOW
#define DIO_SET_DIGITAL_3_LOW DIO_SET_PD3_LOW
#define DIO_SET_DIGITAL_4_LOW DIO_SET_PD4_LOW
#define DIO_SET_DIGITAL_5_LOW DIO_SET_PD5_LOW
#define DIO_SET_DIGITAL_6_LOW DIO_SET_PD6_LOW
#define DIO_SET_DIGITAL_7_LOW DIO_SET_PD7_LOW
#define DIO_SET_DIGITAL_8_LOW DIO_SET_PB0_LOW
#define DIO_SET_DIGITAL_9_LOW DIO_SET_PB1_LOW
#define DIO_SET_DIGITAL_10_LOW DIO_SET_PB2_LOW
#define DIO_SET_DIGITAL_11_LOW DIO_SET_PB3_LOW
#define DIO_SET_DIGITAL_12_LOW DIO_SET_PB4_LOW
#define DIO_SET_DIGITAL_13_LOW DIO_SET_PB5_LOW

#define DIO_SET_DIGITAL_0_HIGH DIO_SET_PD0_HIGH
#define DIO_SET_DIGITAL_1_HIGH DIO_SET_PD1_HIGH
#define DIO_SET_DIGITAL_2_HIGH DIO_SET_PD2_HIGH
#define DIO_SET_DIGITAL_3_HIGH DIO_SET_PD3_HIGH
#define DIO_SET_DIGITAL_4_HIGH DIO_SET_PD4_HIGH
#define DIO_SET_DIGITAL_5_HIGH DIO_SET_PD5_HIGH
#define DIO_SET_DIGITAL_6_HIGH DIO_SET_PD6_HIGH
#define DIO_SET_DIGITAL_7_HIGH DIO_SET_PD7_HIGH
#define DIO_SET_DIGITAL_8_HIGH DIO_SET_PB0_HIGH
#define DIO_SET_DIGITAL_9_HIGH DIO_SET_PB1_HIGH
#define DIO_SET_DIGITAL_10_HIGH DIO_SET_PB2_HIGH
#define DIO_SET_DIGITAL_11_HIGH DIO_SET_PB3_HIGH
#define DIO_SET_DIGITAL_12_HIGH DIO_SET_PB4_HIGH
#define DIO_SET_DIGITAL_13_HIGH DIO_SET_PB5_HIGH

#define DIO_SET_DIGITAL_0 DIO_SET_PD0
#define DIO_SET_DIGITAL_1 DIO_SET_PD1
#define DIO_SET_DIGITAL_2 DIO_SET_PD2
#define DIO_SET_DIGITAL_3 DIO_SET_PD3
#define DIO_SET_DIGITAL_4 DIO_SET_PD4
#define DIO_SET_DIGITAL_5 DIO_SET_PD5
#define DIO_SET_DIGITAL_6 DIO_SET_PD6
#define DIO_SET_DIGITAL_7 DIO_SET_PD7
#define DIO_SET_DIGITAL_8 DIO_SET_PB0
#define DIO_SET_DIGITAL_9 DIO_SET_PB1
#define DIO_SET_DIGITAL_10 DIO_SET_PB2
#define DIO_SET_DIGITAL_11 DIO_SET_PB3
#define DIO_SET_DIGITAL_12 DIO_SET_PB4
#define DIO_SET_DIGITAL_13 DIO_SET_PB5

#define DIO_READ_DIGITAL_0 DIO_READ_PD0
#define DIO_READ_DIGITAL_1 DIO_READ_PD1
#define DIO_READ_DIGITAL_2 DIO_READ_PD2
#define DIO_READ_DIGITAL_3 DIO_READ_PD3
#define DIO_READ_DIGITAL_4 DIO_READ_PD4
#define DIO_READ_DIGITAL_5 DIO_READ_PD5
#define DIO_READ_DIGITAL_6 DIO_READ_PD6
#define DIO_READ_DIGITAL_7 DIO_READ_PD7
#define DIO_READ_DIGITAL_8 DIO_READ_PB0
#define DIO_READ_DIGITAL_9 DIO_READ_PB1
#define DIO_READ_DIGITAL_10 DIO_READ_PB2
#define DIO_READ_DIGITAL_11 DIO_READ_PB3
#define DIO_READ_DIGITAL_12 DIO_READ_PB4
#define DIO_READ_DIGITAL_13 DIO_READ_PB5

// }}}1

#endif // DIO_H
