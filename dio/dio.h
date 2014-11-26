// Simple digital IO macros to hide some of the ugliness of port bit value
// manipulation.

// Test driver: dio_test.c    Implementation: This file

// vim: foldmethod=marker

#ifndef DIO_H
#define DIO_H

#include <avr/io.h>
#include <inttypes.h>

#include "util.h"

///////////////////////////////////////////////////////////////////////////////
//
// How This Interface Works
//
// This interface provides pin-at-a-time initialization, reading, writing,
// and pin change interrupt control of the digital IO pins of the ATMega 328P.
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
//   DIO_INIT_PB0 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
//   DIO_INIT_PB1 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
//
//   uint8_t pb0_value = some_function_returing_boolean ();
//   DIO_SET_PB0 (pb0_value);
//   uint8_t pb1_value = DIO_READ_PB1 ();
//
// When setting a pin to a value known at compile time, it's a tiny bit
// faster (and smaller code) to set the value like this:
//
//   DIO_SET_PB0_HIGH ();
//
// I find these macros easier to read and remember than the individual
// bit fiddling instructions and wait-till-set code.
//
// This interface doesn't support configuring/reading/writing multiple pins
// that use the same port in a single instruction, which is possible with
// the raw memory map read/write interface provided by AVR Libc.
//
// For those who like the alternate numbering system that Aruidnos use for
// their preferred digital IO pins, macros like this are also provided:
//
//   DIO_INIT_DIGITAL_8 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
//
// All caveats described for the underlying macros apply.
//
// There are also macros that expand to tuples of the various register
// and bit macros associated with each pin, and variadic macros which can
// accept these tuple macros as arguments.  So it's also possible to write
// things like this:
//
//   DIO_INIT (DIO_PIN_PB0, DIO_OUTPUT, DIO_DONT_CARE, LOW);
//   DIO_SET (DIO_PIN_PB0, HIGH);
//   DIO_SET_LOW (DIO_PIN_PB0);
//   DIO_SET_HIGH (DIO_PIN_PB0);
//   DIO_INIT (DIO_PIN_PB1, DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
//   uint8_t pb1_value = DIO_READ (DIO_PIN_PB1);
//
// I find these forms harder to read, and the macros are variadic, which
// means you're likely to get strange hard-to-interpret errors if you call
// them wrong.  However, they are convenient for clients that want to allow
// compile-time pin choices.  See the sd_card module for an example.
//
// Finally, macros are also provided to enable pin change interrupts using
// the tuple macros.  For example:
//
// #include <avr/interrupt.h>
// FIXME: I guess according to our principle of direct inclusion we should be
// including stdint.h everywhere, ug.  Or else define byte in util.h :)
// #include <stdint.h>
// #include "dio.h"
//
// #define MY_FAVORITE_PIN DIO_PIN_PB0
//
// volatile uint8_t some_flag = 0;
//
// // Note that each pin group "shares" an interrupt (see comment below).
// // DIO_PIN_CHANGE_VECTOR(PIN) expands to the AVR libc interrupt vector name
// // associated with the pin, so it can be used wherever that vector would be
// // used (i.e. together with AVR libc ISR attributes, EMPTY_INTERRUPT(),
// // etc.).
// ISR (DIO_PIN_CHANGE_VECTOR (MY_FAVORITE_PIN), ISR_NONBLOCK)
// {
//   some_flag = 1;
// }
//
// int
// main (void)
// {
//   DIO_INIT (MY_FAVORITE_PIN, DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
//
//   // This first ensure that the interrupt flag is clear, but *does not*
//   // enable interrupts globally.  Note that each group of pins "shares" an
//   // interrupt, though that interrupt will only be generated if interrupt
//   // generation is enabled for the particular pin experiencing the change.
//   DIO_ENABLE_PIN_CHANGE_INTERRUPT (MY_FAVORITE_PIN);
//
//   sei ();   // Enable interrupts
// }
//

///////////////////////////////////////////////////////////////////////////////
//
// Notes About Particular Pins
//
// All the IO pins on the ATMega chips can be set to perform alternate
// functions other than general digital IO, or controlled in alternate ways
// as described in section 13.3 of the ATMegs328P datasheet Rev. 8271C.
// This interface assumes the normal settings for the Arduino are in effect.
// These are mostly the default setting for the ATMega.  However, the
// Arduino uses some pins for its own pre-set hardware or software purposes:
//
//   * PB3, PB4     used for in-system programming (ISP)
//
//   * PB5          Connected to to ground via ~1 kohm resistor and LED,
//                  with a op amp buffer in there as well for the Uno Rev
//                  3 (and probably later models).  Also used for in-system
//                  programming (ISP)
//
//   * PB6, PB7     used for the external oscillator
//
//   * PC6          used as the reset pin, and for in-system programming
//
//   * PD0, PD1     set up for serial communication by Arduino bootloader,
//                  physically connected to the serial-to-USB interface
//
// These pins cannot be used for arbitrary general digital IO in the normal
// Arduino configuration.  PB5 can be usefully set for output to control
// the onboard LED.
//
// For completeness and easy migration to simpler hardware, this interface
// provides macros for these pins.  At least the following considerations
// apply to their use:
//
//   * PB3 and PB4 can be used, although tying them firmly low or high will
//     prevent in-system programming (ISP) from working.  I don't fully
//     understand ISP/digital IO interaction.
//
//   * PB5 can be used, provided one doesn't mind it being pulled strongly low
//     and/or driving the on-board LED as a side effect (note that activating
//     the internal pull-up resistor will result in a pointless current
//     drain and still fail to pull the input high).  Also, tying it firmly
//     low or high will prevent in-system programming (ISP) from working.
//
//   * PD0 and PD1 can be utilized as general digital IO when using
//     an in-system programmer (instead of serial programming via the
//     bootloader).  Note that the bootloader must be fully removed (which
//     ISP does) or the pins reconfigured in software before this can work.
//     The DIO_INIT_PD0/DIO_INIT_PD1 macros are not sufficient.
//
//   * PB6, PB7 and PC6 cannot be used for general digital IO without
//     hardware and/or fuse bit changes.


///////////////////////////////////////////////////////////////////////////////
//
// Pin Initialization Details
//
// In general, it is possible to re-initialize a pin to change it from
// input to output or vice versa, or to enable or disable its internal
// pull-up resistor.  However, there is one issue that results from the
// order in which the initialiazation macros do things.
//
// When initializing a pin for input, the pin is first set for input, then
// the internal pullup resistor enabled if requested.  Due to the fact that
// the PORTxn bits are recycled and interpreted differently depending on the
// data direction (as set by the DDxn bits), this order of operations means
// that the pin might float for a few microseconds (which might possibly
// result in a spurious pin change interrupt if those are enabled and
// there is no external pull-up or pull-down resister).  We do things in
// this order to ensure that we don't have to risk a spurious change to the
// output value in case the pin is being reconfigured from output to input.
// When initializing a pin for output, the requested initial value is first
// set and then the pin direction set for output.  This might likewise result
// in a momentary deactivation (or activation) of the internal pull-up, and
// a momentarily floating input pin (and potential interrupt).  See Section
// 3.2.3 ATmega328P datasheet, Rev. 8271C, for more details and a possible
// solution if this is an issue.
//
// It's presumably possible to use a pin as a digital input (via the dio
// module or equivalent) sometimes, and as an ADC input other times (via
// the adc module or equivalent).  Note however that the adc_pin_init()
// function of the adc module sets the appropriate DIDR0 bit, and this
// interface doesn't do anything to clear it.  Note also that pins ADC[3..0]
// generate lots of noise on the ADC if they switch while the ADC is in use
// (ATmega datasheet, Rev. 8271C, section 3.6.2)
//
// FIXME: possibly all the loop_until_bit_is_* calls could be replaced with
// single *hardware* no-ops.  Recent versions of AVR libc have added a _NOP
// macro in avr/cpufunc.h that would probably work.

// Ensure that HIGH and LOW are defined as expected.  We could probably make
// due with the normal C definition of truth, but I like to be paranoid
// and keep it the same everywhere in case someone likes to write
// 'if ( some_boolean == TRUE )'.
#if HIGH != 0x01
#  error uh oh, HIGH != 0x01
#endif
#if LOW != 0x00
#  error uh oh, LOW != 0x00
#endif

// Pin Initialization {{{1

// These macros can be used to make DIO_INIT_* calls more readable.
#define DIO_INPUT TRUE
#define DIO_OUTPUT FALSE
#define DIO_ENABLE_PULLUP TRUE
#define DIO_DISABLE_PULLUP FALSE
#define DIO_DONT_CARE FALSE

// Packaged names for the register and bit macros associated with IO pins.
// These can be used by clients that want to let their clients select which
// pin to use at compile-time (see also the DIO_INIT(), DIO_SET_LOW(),
// DIO_SET_HIGH(), DIO_SET(), and DIO_READ() macros).  NOTE: pin change
// interrupts are shared between groups of pins.  NOTE: to support the
// use of these tuples, a number of macros have a variadic top layer.
// Obviously these tuples are best interpreted by the variadic macros
// intented to receive them.
#define DIO_PIN_PB0 DDRB, DDB0, PORTB, PORTB0, PINB, PINB0, \
                    PCIE0, PCIF0, PCMSK0, PCINT0, PCINT0_vect
#define DIO_PIN_PB1 DDRB, DDB1, PORTB, PORTB1, PINB, PINB1 \
                    PCIE0, PCIF0, PCMSK0, PCINT1, PCINT0_vect
#define DIO_PIN_PB2 DDRB, DDB2, PORTB, PORTB2, PINB, PINB2 \
                    PCIE0, PCIF0, PCMSK0, PCINT2, PCINT0_vect
#define DIO_PIN_PB3 DDRB, DDB3, PORTB, PORTB3, PINB, PINB3 \
                    PCIE0, PCIF0, PCMSK0, PCINT3, PCINT0_vect
#define DIO_PIN_PB4 DDRB, DDB4, PORTB, PORTB4, PINB, PINB4 \
                    PCIE0, PCIF0, PCMSK0, PCINT4, PCINT0_vect
#define DIO_PIN_PB5 DDRB, DDB5, PORTB, PORTB5, PINB, PINB5 \
                    PCIE0, PCIF0, PCMSK0, PCINT5, PCINT0_vect
#define DIO_PIN_PB6 DDRB, DDB6, PORTB, PORTB6, PINB, PINB6 \
                    PCIE0, PCIF0, PCMSK0, PCINT6, PCINT0_vect
#define DIO_PIN_PB7 DDRB, DDB7, PORTB, PORTB7, PINB, PINB7 \
                    PCIE0, PCIF0, PCMSK0, PCINT7, PCINT0_vect

#define DIO_PIN_PC0 DDRC, DDC0, PORTC, PORTC0, PINC, PINC0 \
                    PCIE1, PCIF1, PCMSK1, PCINT8, PCINT1_vect
#define DIO_PIN_PC1 DDRC, DDC1, PORTC, PORTC1, PINC, PINC1 \
                    PCIE1, PCIF1, PCMSK1, PCINT9, PCINT1_vect
#define DIO_PIN_PC2 DDRC, DDC2, PORTC, PORTC2, PINC, PINC2 \
                    PCIE1, PCIF1, PCMSK1, PCINT10, PCINT1_vect
#define DIO_PIN_PC3 DDRC, DDC3, PORTC, PORTC3, PINC, PINC3 \
                    PCIE1, PCIF1, PCMSK1, PCINT11, PCINT1_vect
#define DIO_PIN_PC4 DDRC, DDC4, PORTC, PORTC4, PINC, PINC4 \
                    PCIE1, PCIF1, PCMSK1, PCINT12, PCINT1_vect
#define DIO_PIN_PC5 DDRC, DDC5, PORTC, PORTC5, PINC, PINC5 \
                    PCIE1, PCIF1, PCMSK1, PCINT13, PCINT1_vect
#define DIO_PIN_PC6 DDRC, DDC6, PORTC, PORTC6, PINC, PINC6 \
                    PCIE1, PCIF1, PCMSK1, PCINT14, PCINT1_vect

#define DIO_PIN_PD0 DDRD, DDD0, PORTD, PORTD0, PIND, PIND0 \
                    PCIE2, PCIF2, PCMSK2, PCINT16, PCINT2_vect
#define DIO_PIN_PD1 DDRD, DDD1, PORTD, PORTD1, PIND, PIND1 \
                    PCIE2, PCIF2, PCMSK2, PCINT17, PCINT2_vect
#define DIO_PIN_PD2 DDRD, DDD2, PORTD, PORTD2, PIND, PIND2 \
                    PCIE2, PCIF2, PCMSK2, PCINT18, PCINT2_vect
#define DIO_PIN_PD3 DDRD, DDD3, PORTD, PORTD3, PIND, PIND3 \
                    PCIE2, PCIF2, PCMSK2, PCINT19, PCINT2_vect
#define DIO_PIN_PD4 DDRD, DDD4, PORTD, PORTD4, PIND, PIND4 \
                    PCIE2, PCIF2, PCMSK2, PCINT20, PCINT2_vect
#define DIO_PIN_PD5 DDRD, DDD5, PORTD, PORTD5, PIND, PIND5 \
                    PCIE2, PCIF2, PCMSK2, PCINT21, PCINT2_vect
#define DIO_PIN_PD6 DDRD, DDD6, PORTD, PORTD6, PIND, PIND6 \
                    PCIE2, PCIF2, PCMSK2, PCINT22, PCINT2_vect
#define DIO_PIN_PD7 DDRD, DDD7, PORTD, PORTD7, PIND, PIND7 \
                    PCIE2, PCIF2, PCMSK2, PCINT23, PCINT2_vect

// Set a pin (which must have been initialized for output) low.  The argument
// is intended to be one of the DIO_PIN_P* tuples.
#define DIO_SET_LOW(...) DIO_SET_LOW_NA (__VA_ARGS__)

// Underlying named-argument macro implementing the DIO_SET_LOW() macro.
#define DIO_SET_LOW_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ) \
  do { \
    port_reg &= ~(_BV (port_bit)); \
    loop_until_bit_is_clear (port_reg, port_bit); \
  } while ( 0 )

// Set a pin (which must have been initialized for output) high.  This macro
// is intended to take one of the DIO_PIN_P* tuples as its argument.
#define DIO_SET_HIGH(...) DIO_SET_HIGH_NA (__VA_ARGS__)

// Underlying named-argument macro implementing the DIO_SET_HIGH() macro.
#define DIO_SET_HIGH_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ) \
  do { \
    port_reg |= _BV (port_bit); \
    loop_until_bit_is_set (port_reg, port_bit); \
  } while ( 0 )

// Set a pin (which must already be initialized for output) to the value
// given as the last argument.  The first argument is intended to be one
// of the DIO_PIN_P* tuples.
#define DIO_SET(...) DIO_SET_NA (__VA_ARGS__)

// Underlying named-argument macro implementing the DIO_SET() macro.
#define DIO_SET_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect, \
    value ) \
  do { \
    if ( value ) { \
      DIO_SET_HIGH_NA ( \
          dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
          pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ); \
    } \
    else { \
      DIO_SET_LOW_NA ( \
          dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
          pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ); \
    } \
  } while ( 0 )

// Initialize the pin given as the first argument (which should be one of
// the DIO_PIN_P_* tuples) according to the next three arguments
// (see DIO_INIT_NA() for details on these).
#define DIO_INIT(...) DIO_INIT_NA (__VA_ARGS__)

// Underlying named-argument macro implementing the DIO_INIT() macro.
// FIXME: "for_input" doesn't do a good job of suggesting that DIO_INPUT or
// DIO_OUTPUT should be used.  likewise for enable_pullup sort of.  FIXME:
// it would be interesting to know if break; is in anyway more efficient that
// the while (0) that follow it, in any circumstances, with a modern gcc.
// Likewise for not using our other macros inside the if ( enable_pullup
// ) section.
#define DIO_INIT_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect, \
    for_input, enable_pullup, initial_value ) \
  do { \
      if ( for_input ) { \
        dir_reg &= ~(_BV (dir_bit)); \
        loop_until_bit_is_clear (dir_reg, dir_bit); \
        if ( enable_pullup ) { \
          port_reg |= _BV (port_bit); \
          loop_until_bit_is_set (port_reg, port_bit); \
        } \
        else { \
          port_reg &= ~(_BV (port_bit)); \
          loop_until_bit_is_clear (port_reg, port_bit); \
        } \
      } \
      else { \
        DIO_SET ( \
            dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
            pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect, \
            initial_value ); \
        dir_reg |= _BV (dir_bit); \
        loop_until_bit_is_set (dir_reg, dir_bit); \
      } \
      break; \
  } while ( 0 )

// Pin PB* Initialization {{{2

#define DIO_INIT_PB0(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB0, for_input, enable_pullup, initial_value)

#define DIO_INIT_PB1(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB1, for_input, enable_pullup, initial_value)

#define DIO_INIT_PB2(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB2, for_input, enable_pullup, initial_value)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB3(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB3, for_input, enable_pullup, initial_value)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB4(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB4, for_input, enable_pullup, initial_value)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB5(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB5, for_input, enable_pullup, initial_value)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB6(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB6, for_input, enable_pullup, initial_value)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PB7(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PB7, for_input, enable_pullup, initial_value)

/// }}}2

// Pin PC* Initialization {{{2

#define DIO_INIT_PC0(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PC0, for_input, enable_pullup, initial_value)

#define DIO_INIT_PC1(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PC1, for_input, enable_pullup, initial_value)

#define DIO_INIT_PC2(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PC2, for_input, enable_pullup, initial_value)

#define DIO_INIT_PC3(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PC3, for_input, enable_pullup, initial_value)

#define DIO_INIT_PC4(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PC4, for_input, enable_pullup, initial_value)

#define DIO_INIT_PC5(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PC5, for_input, enable_pullup, initial_value)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PC6(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PC6, for_input, enable_pullup, initial_value)

// }}}2

// Pin PD* Initialization {{{2

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PD0(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD0, for_input, enable_pullup, initial_value)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_INIT_PD1(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD1, for_input, enable_pullup, initial_value)

#define DIO_INIT_PD2(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD2, for_input, enable_pullup, initial_value)

#define DIO_INIT_PD3(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD3, for_input, enable_pullup, initial_value)

#define DIO_INIT_PD4(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD4, for_input, enable_pullup, initial_value)

#define DIO_INIT_PD5(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD5, for_input, enable_pullup, initial_value)

#define DIO_INIT_PD6(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD6, for_input, enable_pullup, initial_value)

#define DIO_INIT_PD7(for_input, enable_pullup, initial_value) \
  DIO_INIT (DIO_PIN_PD7, for_input, enable_pullup, initial_value)

// }}}2

// }}}1

// Pin Setting {{{1

// Pins PB* Without Argument {{{2

#define DIO_SET_PB0_LOW() DIO_SET_LOW (DIO_PIN_PB0)
#define DIO_SET_PB0_HIGH() DIO_SET_HIGH (DIO_PIN_PB0)

#define DIO_SET_PB1_LOW() DIO_SET_LOW (DIO_PIN_PB1)
#define DIO_SET_PB1_HIGH() DIO_SET_HIGH (DIO_PIN_PB1)

#define DIO_SET_PB2_LOW() DIO_SET_LOW (DIO_PIN_PB2)
#define DIO_SET_PB2_HIGH() DIO_SET_HIGH (DIO_PIN_PB2)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB3_LOW() DIO_SET_LOW (DIO_PIN_PB3)
#define DIO_SET_PB3_HIGH() DIO_SET_HIGH (DIO_PIN_PB3)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB4_LOW() DIO_SET_LOW (DIO_PIN_PB4)
#define DIO_SET_PB4_HIGH() DIO_SET_HIGH (DIO_PIN_PB4)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB5_LOW() DIO_SET_LOW (DIO_PIN_PB5)
#define DIO_SET_PB5_HIGH() DIO_SET_HIGH (DIO_PIN_PB5)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB6_LOW() DIO_SET_LOW (DIO_PIN_PB6)
#define DIO_SET_PB6_HIGH() DIO_SET_HIGH (DIO_PIN_PB6)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB7_LOW() DIO_SET_LOW (DIO_PIN_PB7)
#define DIO_SET_PB7_HIGH() DIO_SET_HIGH (DIO_PIN_PB7)

// }}}2

// Pins PC* Without Argument {{{2

#define DIO_SET_PC0_LOW() DIO_SET_LOW (DIO_PIN_PC0)
#define DIO_SET_PC0_HIGH() DIO_SET_HIGH (DIO_PIN_PC0)

#define DIO_SET_PC1_LOW() DIO_SET_LOW (DIO_PIN_PC1)
#define DIO_SET_PC1_HIGH() DIO_SET_HIGH (DIO_PIN_PC1)

#define DIO_SET_PC2_LOW() DIO_SET_LOW (DIO_PIN_PC2)
#define DIO_SET_PC2_HIGH() DIO_SET_HIGH (DIO_PIN_PC2)

#define DIO_SET_PC3_LOW() DIO_SET_LOW (DIO_PIN_PC3)
#define DIO_SET_PC3_HIGH() DIO_SET_HIGH (DIO_PIN_PC3)

#define DIO_SET_PC4_LOW() DIO_SET_LOW (DIO_PIN_PC4)
#define DIO_SET_PC4_HIGH() DIO_SET_HIGH (DIO_PIN_PC4)

#define DIO_SET_PC5_LOW() DIO_SET_LOW (DIO_PIN_PC5)
#define DIO_SET_PC5_HIGH() DIO_SET_HIGH (DIO_PIN_PC5)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PC6_LOW() DIO_SET_LOW (DIO_PIN_PC6)
#define DIO_SET_PC6_HIGH() DIO_SET_HIGH (DIO_PIN_PC6)

// }}}2

// Pins PD* Without Argument {{{2

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD0_LOW() DIO_SET_LOW (DIO_PIN_PD0)
#define DIO_SET_PD0_HIGH() DIO_SET_HIGH (DIO_PIN_PD0)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD1_LOW() DIO_SET_LOW (DIO_PIN_PD1)
#define DIO_SET_PD1_HIGH() DIO_SET_HIGH (DIO_PIN_PD1)

#define DIO_SET_PD2_LOW() DIO_SET_LOW (DIO_PIN_PD2)
#define DIO_SET_PD2_HIGH() DIO_SET_HIGH (DIO_PIN_PD2)

#define DIO_SET_PD3_LOW() DIO_SET_LOW (DIO_PIN_PD3)
#define DIO_SET_PD3_HIGH() DIO_SET_HIGH (DIO_PIN_PD3)

#define DIO_SET_PD4_LOW() DIO_SET_LOW (DIO_PIN_PD4)
#define DIO_SET_PD4_HIGH() DIO_SET_HIGH (DIO_PIN_PD4)

#define DIO_SET_PD5_LOW() DIO_SET_LOW (DIO_PIN_PD5)
#define DIO_SET_PD5_HIGH() DIO_SET_HIGH (DIO_PIN_PD5)

#define DIO_SET_PD6_LOW() DIO_SET_LOW (DIO_PIN_PD6)
#define DIO_SET_PD6_HIGH() DIO_SET_HIGH (DIO_PIN_PD6)

#define DIO_SET_PD7_LOW() DIO_SET_LOW (DIO_PIN_PD7)
#define DIO_SET_PD7_HIGH() DIO_SET_HIGH (DIO_PIN_PD7)

// }}}2

// Pins PB* With Argument {{{2

#define DIO_SET_PB0(value) DIO_SET (DIO_PIN_PB0, value)
#define DIO_SET_PB1(value) DIO_SET (DIO_PIN_PB1, value)
#define DIO_SET_PB2(value) DIO_SET (DIO_PIN_PB2, value)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB3(value) DIO_SET (DIO_PIN_PB3, value)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB4(value) DIO_SET (DIO_PIN_PB4, value)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB5(value) DIO_SET (DIO_PIN_PB5, value)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB6(value) DIO_SET (DIO_PIN_PB6, value)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PB7(value) DIO_SET (DIO_PIN_PB7, value)

// }}}2

// Pins PC* With Argument {{{2

#define DIO_SET_PC0(value) DIO_SET (DIO_PIN_PC0, value)
#define DIO_SET_PC1(value) DIO_SET (DIO_PIN_PC1, value)
#define DIO_SET_PC2(value) DIO_SET (DIO_PIN_PC2, value)
#define DIO_SET_PC3(value) DIO_SET (DIO_PIN_PC3, value)
#define DIO_SET_PC4(value) DIO_SET (DIO_PIN_PC4, value)
#define DIO_SET_PC5(value) DIO_SET (DIO_PIN_PC5, value)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PC6(value) DIO_SET (DIO_PIN_PC6, value)

// }}}2

// Pins PD* With Argument {{{2

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD0(value) DIO_SET (DIO_PIN_PD0, value)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_SET_PD1(value) DIO_SET (DIO_PIN_PD1, value)
#define DIO_SET_PD2(value) DIO_SET (DIO_PIN_PD2, value)
#define DIO_SET_PD3(value) DIO_SET (DIO_PIN_PD3, value)
#define DIO_SET_PD4(value) DIO_SET (DIO_PIN_PD4, value)
#define DIO_SET_PD5(value) DIO_SET (DIO_PIN_PD5, value)
#define DIO_SET_PD6(value) DIO_SET (DIO_PIN_PD6, value)
#define DIO_SET_PD7(value) DIO_SET (DIO_PIN_PD7, value)

// }}}2

// }}}1

// Pin Reading {{{1

// Read a pin (which must have been initialized for input).  This macro is
// intended to take one of the DIO_PIN_P* tuples as its argument.
#define DIO_READ(...) DIO_READ_NA(__VA_ARGS__)

// Underlying named-argument macro implementing the DIO_READ() macro.  NOTE:
// The bit shifts are only included so that the results will work in case
// the user actually compares the result to the value of the 'HIGH' macro.
// We probably waste an instruction doing this, but the Arduino libraries
// define HIGH this way and it's kind of nice to keep everything symbolic,
// so we do the same.
#define DIO_READ_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ) \
  ((pin_reg & _BV (pin_bit)) >> pin_bit)

#define DIO_READ_PB0() DIO_READ (DIO_PIN_PB0)
#define DIO_READ_PB1() DIO_READ (DIO_PIN_PB1)
#define DIO_READ_PB2() DIO_READ (DIO_PIN_PB2)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB3() DIO_READ (DIO_PIN_PB3)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB4() DIO_READ (DIO_PIN_PB4)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB5() DIO_READ (DIO_PIN_PB5)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB6() DIO_READ (DIO_PIN_PB6)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PB7() DIO_READ (DIO_PIN_PB7)

#define DIO_READ_PC0() DIO_READ (DIO_PIN_PC0)
#define DIO_READ_PC1() DIO_READ (DIO_PIN_PC1)
#define DIO_READ_PC2() DIO_READ (DIO_PIN_PC2)
#define DIO_READ_PC3() DIO_READ (DIO_PIN_PC3)
#define DIO_READ_PC4() DIO_READ (DIO_PIN_PC4)
#define DIO_READ_PC5() DIO_READ (DIO_PIN_PC5)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PC6() DIO_READ (DIO_PIN_PC6)

// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PD0() DIO_READ (DIO_PIN_PD0)
// WARNING: See comments in 'Notes About Particular Pins' above.
#define DIO_READ_PD1() DIO_READ (DIO_PIN_PD1)
#define DIO_READ_PD2() DIO_READ (DIO_PIN_PD2)
#define DIO_READ_PD3() DIO_READ (DIO_PIN_PD3)
#define DIO_READ_PD4() DIO_READ (DIO_PIN_PD4)
#define DIO_READ_PD5() DIO_READ (DIO_PIN_PD5)
#define DIO_READ_PD6() DIO_READ (DIO_PIN_PD6)
#define DIO_READ_PD7() DIO_READ (DIO_PIN_PD7)

// }}}1

// Pin Change Interrupt Control {{{1

// Clear the interrupt flag, enable the interrupt for the pin group containing
// the pin, and set the mask bit for the pin such that interrupts will be
// generated when it changes.  Note that this enables interrupts for *all*
// the pins in the pin group that have their mask bits set.  This macro is
// intended to take one of the DIO_PIN_P* tuples as its argument.
#define DIO_ENABLE_PIN_CHANGE_INTERRUPT(...) \
  DIO_ENABLE_PIN_CHANGE_INTERRUPT_NA (__VA_ARGS__)

// Underlying named-argument macro implementing the
// DIO_ENABLE_PIN_CHANGE_INTERRUPT() macro.  Note that pcif_bit is actually
// *cleared* by writing a logical one to it.
#define DIO_ENABLE_PIN_CHANGE_INTERRUPT_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ) \
  do { \
    PCIFR |= _BV (pcif_bit); \
    loop_until_bit_is_clear (PCIFR, pcif_bit); \
    PCICR |= _BV (pcie_bit); \
    loop_until_bit_is_set (PCICR, pcie_bit); \
    pcmsk_reg |= _BV (pcint_bit); \
    loop_until_bit_is_set (pcmsk_reg, pcint_bit); \
  } while ( 0 )

// Clear the mask bit the the pin and disable the interrupt for the pin
// group containing the pin.  Note that this disables the shared interrupt
// for *all* the pins in the pin group.  Finally, ensure that the interrupt
// flag for the pin group is clear.  This macro is intended to take one of
// the DIO_PIN_P* tuples as its argument.
#define DIO_DISABLE_PIN_CHANGE_INTERRUPT(...) \
  DIO_DISABLE_PIN_CHANGE_INTERRUPT_NA(__VA_ARGS__) \

// Underlying named-argument macro implementing the
// DIO_DISABLE_PIN_CHANGE_INTERRUPT() macro.  Note that pcif_bit is actually
// *cleared* by writing a logical one to it.
#define DIO_DISABLE_PIN_CHANGE_INTERRUPT_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ) \
  do { \
    pcmsk_reg &= ~(_BV (pcint_bit)); \
    loop_until_bit_is_clear (pcmsk_reg, pcint_bit); \
    PCICR &= ~(_BV (pcie_bit)); \
    loop_until_bit_is_clear (PCICR, pcie_bit); \
    PCIFR |= _BV (pcif_bit); \
    loop_until_bit_is_clear (PCIFR, pcif_bit); \
  } while ( 0 )

// The name of the interrupt vector as AVR libc knows it.  The expansion
// of this macro is acceptable as an argument to AVR libc's ISR(),
// EMPTY_INTERRUPT(), or ISR_ALIAS() macros.  This macro is intended to
// take one of the DIO_PIN_P* tuples as its argument.
#define DIO_PIN_CHANGE_INTERRUPT_VECTOR(...) \
  DIO_PIN_CHANGE_INTERRUPT_VECTOR_NA(__VA_ARGS__)

// Underlying named-argument macro implementing the DIO_PIN_CHANGE_VECTOR()
// macro.
#define DIO_PIN_CHANGE_INTERRUPT_VECTOR_NA( \
    dir_reg, dir_bit, port_reg, port_bit, pin_reg, pin_bit, \
    pcie_bit, pcif_bit, pcmsk_reg, pcint_bit, pcint_vect ) \
  pcint_vect

// FIXME: maybe we want per-pin mask control macros?

// }}}1

// Arduino Compatible Pin Number Macros {{{1

// WARNING: all caveats described for the underlying macros apply.

#define DIO_PIN_DIGITAL_0  DIO_PIN_PD0
#define DIO_PIN_DIGITAL_1  DIO_PIN_PD1
#define DIO_PIN_DIGITAL_2  DIO_PIN_PD2
#define DIO_PIN_DIGITAL_3  DIO_PIN_PD3
#define DIO_PIN_DIGITAL_4  DIO_PIN_PD4
#define DIO_PIN_DIGITAL_5  DIO_PIN_PD5
#define DIO_PIN_DIGITAL_6  DIO_PIN_PD6
#define DIO_PIN_DIGITAL_7  DIO_PIN_PD7
#define DIO_PIN_DIGITAL_8  DIO_PIN_PB0
#define DIO_PIN_DIGITAL_9  DIO_PIN_PB1
#define DIO_PIN_DIGITAL_10 DIO_PIN_PB2
#define DIO_PIN_DIGITAL_11 DIO_PIN_PB3
#define DIO_PIN_DIGITAL_12 DIO_PIN_PB4
#define DIO_PIN_DIGITAL_13 DIO_PIN_PB5

#define DIO_INIT_DIGITAL_0  DIO_INIT_PD0
#define DIO_INIT_DIGITAL_1  DIO_INIT_PD1
#define DIO_INIT_DIGITAL_2  DIO_INIT_PD2
#define DIO_INIT_DIGITAL_3  DIO_INIT_PD3
#define DIO_INIT_DIGITAL_4  DIO_INIT_PD4
#define DIO_INIT_DIGITAL_5  DIO_INIT_PD5
#define DIO_INIT_DIGITAL_6  DIO_INIT_PD6
#define DIO_INIT_DIGITAL_7  DIO_INIT_PD7
#define DIO_INIT_DIGITAL_8  DIO_INIT_PB0
#define DIO_INIT_DIGITAL_9  DIO_INIT_PB1
#define DIO_INIT_DIGITAL_10 DIO_INIT_PB2
#define DIO_INIT_DIGITAL_11 DIO_INIT_PB3
#define DIO_INIT_DIGITAL_12 DIO_INIT_PB4
#define DIO_INIT_DIGITAL_13 DIO_INIT_PB5

#define DIO_SET_DIGITAL_0_LOW  DIO_SET_PD0_LOW
#define DIO_SET_DIGITAL_1_LOW  DIO_SET_PD1_LOW
#define DIO_SET_DIGITAL_2_LOW  DIO_SET_PD2_LOW
#define DIO_SET_DIGITAL_3_LOW  DIO_SET_PD3_LOW
#define DIO_SET_DIGITAL_4_LOW  DIO_SET_PD4_LOW
#define DIO_SET_DIGITAL_5_LOW  DIO_SET_PD5_LOW
#define DIO_SET_DIGITAL_6_LOW  DIO_SET_PD6_LOW
#define DIO_SET_DIGITAL_7_LOW  DIO_SET_PD7_LOW
#define DIO_SET_DIGITAL_8_LOW  DIO_SET_PB0_LOW
#define DIO_SET_DIGITAL_9_LOW  DIO_SET_PB1_LOW
#define DIO_SET_DIGITAL_10_LOW DIO_SET_PB2_LOW
#define DIO_SET_DIGITAL_11_LOW DIO_SET_PB3_LOW
#define DIO_SET_DIGITAL_12_LOW DIO_SET_PB4_LOW
#define DIO_SET_DIGITAL_13_LOW DIO_SET_PB5_LOW

#define DIO_SET_DIGITAL_0_HIGH  DIO_SET_PD0_HIGH
#define DIO_SET_DIGITAL_1_HIGH  DIO_SET_PD1_HIGH
#define DIO_SET_DIGITAL_2_HIGH  DIO_SET_PD2_HIGH
#define DIO_SET_DIGITAL_3_HIGH  DIO_SET_PD3_HIGH
#define DIO_SET_DIGITAL_4_HIGH  DIO_SET_PD4_HIGH
#define DIO_SET_DIGITAL_5_HIGH  DIO_SET_PD5_HIGH
#define DIO_SET_DIGITAL_6_HIGH  DIO_SET_PD6_HIGH
#define DIO_SET_DIGITAL_7_HIGH  DIO_SET_PD7_HIGH
#define DIO_SET_DIGITAL_8_HIGH  DIO_SET_PB0_HIGH
#define DIO_SET_DIGITAL_9_HIGH  DIO_SET_PB1_HIGH
#define DIO_SET_DIGITAL_10_HIGH DIO_SET_PB2_HIGH
#define DIO_SET_DIGITAL_11_HIGH DIO_SET_PB3_HIGH
#define DIO_SET_DIGITAL_12_HIGH DIO_SET_PB4_HIGH
#define DIO_SET_DIGITAL_13_HIGH DIO_SET_PB5_HIGH

#define DIO_SET_DIGITAL_0  DIO_SET_PD0
#define DIO_SET_DIGITAL_1  DIO_SET_PD1
#define DIO_SET_DIGITAL_2  DIO_SET_PD2
#define DIO_SET_DIGITAL_3  DIO_SET_PD3
#define DIO_SET_DIGITAL_4  DIO_SET_PD4
#define DIO_SET_DIGITAL_5  DIO_SET_PD5
#define DIO_SET_DIGITAL_6  DIO_SET_PD6
#define DIO_SET_DIGITAL_7  DIO_SET_PD7
#define DIO_SET_DIGITAL_8  DIO_SET_PB0
#define DIO_SET_DIGITAL_9  DIO_SET_PB1
#define DIO_SET_DIGITAL_10 DIO_SET_PB2
#define DIO_SET_DIGITAL_11 DIO_SET_PB3
#define DIO_SET_DIGITAL_12 DIO_SET_PB4
#define DIO_SET_DIGITAL_13 DIO_SET_PB5

#define DIO_READ_DIGITAL_0  DIO_READ_PD0
#define DIO_READ_DIGITAL_1  DIO_READ_PD1
#define DIO_READ_DIGITAL_2  DIO_READ_PD2
#define DIO_READ_DIGITAL_3  DIO_READ_PD3
#define DIO_READ_DIGITAL_4  DIO_READ_PD4
#define DIO_READ_DIGITAL_5  DIO_READ_PD5
#define DIO_READ_DIGITAL_6  DIO_READ_PD6
#define DIO_READ_DIGITAL_7  DIO_READ_PD7
#define DIO_READ_DIGITAL_8  DIO_READ_PB0
#define DIO_READ_DIGITAL_9  DIO_READ_PB1
#define DIO_READ_DIGITAL_10 DIO_READ_PB2
#define DIO_READ_DIGITAL_11 DIO_READ_PB3
#define DIO_READ_DIGITAL_12 DIO_READ_PB4
#define DIO_READ_DIGITAL_13 DIO_READ_PB5

// }}}1

#endif // DIO_H
