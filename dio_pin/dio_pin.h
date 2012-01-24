// Simple macros to hide some of the ugliness of port bit value manipulation.

// vim: foldmethod=marker

#ifndef DIO_PIN_H
#define DIO_PIN_H

// FIXME: possibly all the loop_until_bit_is_* calls could be replaced with
// single no-ops.

// Pin Setting {{{1

#define DIO_PIN_SET_PB0_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB0)); \
    loop_until_bit_is_clear (PORTB, PORTB0); \
  } while ( 0 )

#define DIO_PIN_SET_PB0_HIGH() \
  do { \
    PORTB |= _BV (PORTB0); \
    loop_until_bit_is_set (PORTB, PORTB0); \
  } while ( 0 )


#define DIO_PIN_SET_PB1_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB1)); \
    loop_until_bit_is_clear (PORTB, PORTB1); \
  } while ( 0 )

#define DIO_PIN_SET_PB1_HIGH() \
  do { \
    PORTB |= _BV (PORTB1); \
    loop_until_bit_is_set (PORTB, PORTB1); \
  } while ( 0 )


#define DIO_PIN_SET_PB2_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB2)); \
    loop_until_bit_is_clear (PORTB, PORTB2); \
  } while ( 0 )

#define DIO_PIN_SET_PB2_HIGH() \
  do { \
    PORTB |= _BV (PORTB2); \
    loop_until_bit_is_set (PORTB, PORTB2); \
  } while ( 0 )


#define DIO_PIN_SET_PB3_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB3)); \
    loop_until_bit_is_clear (PORTB, PORTB3); \
  } while ( 0 )

#define DIO_PIN_SET_PB3_HIGH() \
  do { \
    PORTB |= _BV (PORTB3); \
    loop_until_bit_is_set (PORTB, PORTB3); \
  } while ( 0 )


#define DIO_PIN_SET_PB4_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB4)); \
    loop_until_bit_is_clear (PORTB, PORTB4); \
  } while ( 0 )

#define DIO_PIN_SET_PB4_HIGH() \
  do { \
    PORTB |= _BV (PORTB4); \
    loop_until_bit_is_set (PORTB, PORTB4); \
  } while ( 0 )


#define DIO_PIN_SET_PB5_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB5)); \
    loop_until_bit_is_clear (PORTB, PORTB5); \
  } while ( 0 )

#define DIO_PIN_SET_PB5_HIGH() \
  do { \
    PORTB |= _BV (PORTB5); \
    loop_until_bit_is_set (PORTB, PORTB5); \
  } while ( 0 )


#define DIO_PIN_SET_PB6_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB6)); \
    loop_until_bit_is_clear (PORTB, PORTB6); \
  } while ( 0 )

#define DIO_PIN_SET_PB6_HIGH() \
  do { \
    PORTB |= _BV (PORTB6); \
    loop_until_bit_is_set (PORTB, PORTB6); \
  } while ( 0 )


#define DIO_PIN_SET_PB7_LOW() \
  do { \
    PORTB &= ~(_BV (PORTB7)); \
    loop_until_bit_is_clear (PORTB, PORTB7); \
  } while ( 0 )

#define DIO_PIN_SET_PB7_HIGH() \
  do { \
    PORTB |= _BV (PORTB7); \
    loop_until_bit_is_set (PORTB, PORTB7); \
  } while ( 0 )




#define DIO_PIN_SET_PC0_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC0)); \
    loop_until_bit_is_clear (PORTC, PORTC0); \
  } while ( 0 )

#define DIO_PIN_SET_PC0_HIGH() \
  do { \
    PORTC |= _BV (PORTC0); \
    loop_until_bit_is_set (PORTC, PORTC0); \
  } while ( 0 )


#define DIO_PIN_SET_PC1_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC1)); \
    loop_until_bit_is_clear (PORTC, PORTC1); \
  } while ( 0 )

#define DIO_PIN_SET_PC1_HIGH() \
  do { \
    PORTC |= _BV (PORTC1); \
    loop_until_bit_is_set (PORTC, PORTC1); \
  } while ( 0 )


#define DIO_PIN_SET_PC2_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC2)); \
    loop_until_bit_is_clear (PORTC, PORTC2); \
  } while ( 0 )

#define DIO_PIN_SET_PC2_HIGH() \
  do { \
    PORTC |= _BV (PORTC2); \
    loop_until_bit_is_set (PORTC, PORTC2); \
  } while ( 0 )


#define DIO_PIN_SET_PC3_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC3)); \
    loop_until_bit_is_clear (PORTC, PORTC3); \
  } while ( 0 )

#define DIO_PIN_SET_PC3_HIGH() \
  do { \
    PORTC |= _BV (PORTC3); \
    loop_until_bit_is_set (PORTC, PORTC3); \
  } while ( 0 )


#define DIO_PIN_SET_PC4_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC4)); \
    loop_until_bit_is_clear (PORTC, PORTC4); \
  } while ( 0 )

#define DIO_PIN_SET_PC4_HIGH() \
  do { \
    PORTC |= _BV (PORTC4); \
    loop_until_bit_is_set (PORTC, PORTC4); \
  } while ( 0 )


#define DIO_PIN_SET_PC5_LOW() \
  do { \
    PORTC &= ~(_BV (PORTC5)); \
    loop_until_bit_is_clear (PORTC, PORTC5); \
  } while ( 0 )

#define DIO_PIN_SET_PC5_HIGH() \
  do { \
    PORTC |= _BV (PORTC5); \
    loop_until_bit_is_set (PORTC, PORTC5); \
  } while ( 0 )




#define DIO_PIN_SET_PD0_HIGH() \
  do { \
    PORTD |= _BV (PORTD0); \
    loop_until_bit_is_set (PORTD, PORTD0); \
  } while ( 0 )

#define DIO_PIN_SET_PD0_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD0)); \
    loop_until_bit_is_clear (PORTD, PORTD0); \
  } while ( 0 )


#define DIO_PIN_SET_PD1_HIGH() \
  do { \
    PORTD |= _BV (PORTD1); \
    loop_until_bit_is_set (PORTD, PORTD1); \
  } while ( 0 )

#define DIO_PIN_SET_PD1_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD1)); \
    loop_until_bit_is_clear (PORTD, PORTD1); \
  } while ( 0 )


#define DIO_PIN_SET_PD2_HIGH() \
  do { \
    PORTD |= _BV (PORTD2); \
    loop_until_bit_is_set (PORTD, PORTD2); \
  } while ( 0 )

#define DIO_PIN_SET_PD2_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD2)); \
    loop_until_bit_is_clear (PORTD, PORTD2); \
  } while ( 0 )


#define DIO_PIN_SET_PD3_HIGH() \
  do { \
    PORTD |= _BV (PORTD3); \
    loop_until_bit_is_set (PORTD, PORTD3); \
  } while ( 0 )

#define DIO_PIN_SET_PD3_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD3)); \
    loop_until_bit_is_clear (PORTD, PORTD3); \
  } while ( 0 )


#define DIO_PIN_SET_PD4_HIGH() \
  do { \
    PORTD |= _BV (PORTD4); \
    loop_until_bit_is_set (PORTD, PORTD4); \
  } while ( 0 )

#define DIO_PIN_SET_PD4_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD4)); \
    loop_until_bit_is_clear (PORTD, PORTD4); \
  } while ( 0 )


#define DIO_PIN_SET_PD5_HIGH() \
  do { \
    PORTD |= _BV (PORTD5); \
    loop_until_bit_is_set (PORTD, PORTD5); \
  } while ( 0 )

#define DIO_PIN_SET_PD5_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD5)); \
    loop_until_bit_is_clear (PORTD, PORTD5); \
  } while ( 0 )


#define DIO_PIN_SET_PD6_HIGH() \
  do { \
    PORTD |= _BV (PORTD6); \
    loop_until_bit_is_set (PORTD, PORTD6); \
  } while ( 0 )

#define DIO_PIN_SET_PD6_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD6)); \
    loop_until_bit_is_clear (PORTD, PORTD6); \
  } while ( 0 )


#define DIO_PIN_SET_PD7_HIGH() \
  do { \
    PORTD |= _BV (PORTD7); \
    loop_until_bit_is_set (PORTD, PORTD7); \
  } while ( 0 )

#define DIO_PIN_SET_PD7_LOW() \
  do { \
    PORTD &= ~(_BV (PORTD7)); \
    loop_until_bit_is_clear (PORTD, PORTD7); \
  } while ( 0 )




#define DIO_PIN_SET_PB0(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB0_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB0_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PB1(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB1_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB1_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PB2(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB2_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB2_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PB3(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB3_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB3_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PB4(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB4_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB4_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PB5(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB5_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB5_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PB6(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB6_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB6_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PB7(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PB7_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PB7_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PC0(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PC0_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PC0_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PC1(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PC1_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PC1_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PC2(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PC2_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PC2_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PC3(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PC3_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PC3_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PC4(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PC4_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PC4_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PC5(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PC5_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PC5_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD0(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD0_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD0_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD1(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD1_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD1_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD2(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD2_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD2_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD3(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD3_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD3_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD4(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD4_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD4_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD5(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD5_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD5_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD6(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD6_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD6_LOW (); \
    } \
  } while ( 0 )

#define DIO_PIN_SET_PD7(value) \
  do { \
    if ( value ) { \
      DIO_PIN_SET_PD7_HIGH (); \
    } \
    else { \
      DIO_PIN_SET_PD7_LOW (); \
    } \
  } while ( 0 )

// Pin Initialization {{{1

// These macros can be used to make DIO_PIN_INIT_* calls more readable.
#define DIO_PIN_INPUT  1
#define DIO_PIN_OUTPUT 0
#define DIO_PIN_ENABLE_PULLUP 1
#define DIO_PIN_DISABLE_PULLUP 1
#define DIO_PIN_DONT_CARE 0

// NOTE: when initializing a pin for input, the pin is first set for input,
// then the internal pullup resistor is enabled.  This means that the pin
// might float for a few microseconds (which might possibly result in a
// spurious pin change interrupt if those are enabled and there is no external
// pull-up or pull-down resister, I'm not sure), but we don't have to risk a
// spurious change to the output value in case the pin is being reconfigured
// from output to input.  When initializing a pin for output, the requested
// initial value is first set and the the pin direction set for output.
// This might likewise result in a momentarily floating input pin (and
// potential interrupt).  FIXME: move this note into place with other docs.
#define DIO_PIN_INIT_PB0(for_input, enable_pullup, initial_value) \
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
        DIO_PIN_SET_PB0 (initial_value); \
        DDRB |= _BV (DDB0); \
        loop_until_bit_is_set (DDRB, DDB0); \
      } \
      break; \
  } while ( 0 )

#define DIO_PIN_INIT_PB1(for_input, enable_pullup, initial_value) \
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
        DIO_PIN_SET_PB1 (initial_value); \
        DDRB |= _BV (DDB1); \
        loop_until_bit_is_set (DDRB, DDB1); \
      } \
      break; \
  } while ( 0 )

// FIXME: fill in the rest of the DIO_PIN_INIT_* functions.

// Pin Reading {{{1

#define DIO_PIN_READ_PB0() (PINB & _BV (PINB0))
#define DIO_PIN_READ_PB1() (PINB & _BV (PINB1))
#define DIO_PIN_READ_PB2() (PINB & _BV (PINB2))
#define DIO_PIN_READ_PB3() (PINB & _BV (PINB3))
#define DIO_PIN_READ_PB4() (PINB & _BV (PINB4))
#define DIO_PIN_READ_PB5() (PINB & _BV (PINB5))
#define DIO_PIN_READ_PB6() (PINB & _BV (PINB6))
#define DIO_PIN_READ_PB7() (PINB & _BV (PINB7))

#define DIO_PIN_READ_PC0() (PINC & _BV (PINC0))
#define DIO_PIN_READ_PC1() (PINC & _BV (PINC1))
#define DIO_PIN_READ_PC2() (PINC & _BV (PINC2))
#define DIO_PIN_READ_PC3() (PINC & _BV (PINC3))
#define DIO_PIN_READ_PC4() (PINC & _BV (PINC4))
#define DIO_PIN_READ_PC5() (PINC & _BV (PINC5))

#define DIO_PIN_READ_PD0() (PIND & _BV (PIND0))
#define DIO_PIN_READ_PD1() (PIND & _BV (PIND1))
#define DIO_PIN_READ_PD2() (PIND & _BV (PIND2))
#define DIO_PIN_READ_PD3() (PIND & _BV (PIND3))
#define DIO_PIN_READ_PD4() (PIND & _BV (PIND4))
#define DIO_PIN_READ_PD5() (PIND & _BV (PIND5))
#define DIO_PIN_READ_PD6() (PIND & _BV (PIND6))
#define DIO_PIN_READ_PD7() (PIND & _BV (PIND7))

// WARNING: Not all the digital IO pins are available for use when using
// an Arduino with the normal bootloader pin configuration.  PD0 and PD1
// are set up for serial communication, and won't work as general digital
// IO pins (at least without disturbing something else).
//
// The pin names we use are the unparenthesized pin labels from the
// ATMega328p datasheet.  The assigned numbers are the digital pin numbers
// printed on (at least some) arduino boards (so they can be used instead if
// the user likes).  For the PB6:7 and PC0:5 pins, my arduino doesn't have
// any specific digital IO-related mark, so these pins have no guaranteed
// value in this enumeration.
typedef enum {
  DIGITAL_IO_PIN_PB0 = 8, 
  DIGITAL_IO_PIN_PB1 = 9,
  DIGITAL_IO_PIN_PB2 = 10, 
  DIGITAL_IO_PIN_PB3 = 11, 
  DIGITAL_IO_PIN_PB4 = 12,
  DIGITAL_IO_PIN_PB5 = 13,
  DIGITAL_IO_PIN_PB6,
  DIGITAL_IO_PIN_PB7,
  DIGITAL_IO_PIN_PC0,
  DIGITAL_IO_PIN_PC1,
  DIGITAL_IO_PIN_PC2,
  DIGITAL_IO_PIN_PC3,
  DIGITAL_IO_PIN_PC4,
  DIGITAL_IO_PIN_PC5,
  DIGITAL_IO_PIN_PD0 = 0,
  DIGITAL_IO_PIN_PD1 = 1, 
  DIGITAL_IO_PIN_PD2 = 2, 
  DIGITAL_IO_PIN_PD3 = 3, 
  DIGITAL_IO_PIN_PD4 = 4, 
  DIGITAL_IO_PIN_PD5 = 5,
  DIGITAL_IO_PIN_PD6 = 6, 
  DIGITAL_IO_PIN_PD7 = 7
} digital_io_pin_name_t;

typedef enum {
  DIGITAL_IO_PIN_DIRECTION_INPUT,
  DIGITAL_IO_PIN_DIRECTION_OUTPUT
} digital_io_pin_direction_t;

// Initialize pin of port for input or output (as per direction argument).
// If the pin is configured for input, then enable_pullup determines whether
// the internal pullup resistor is enabled.  If configured for ouput,
// initial_value determines the initial value of the pin.
void
digital_io_pin_init (
    digital_io_pin_name_t        pin,
    digital_io_pin_direction_t   direction,
    uint8_t                      enable_pullup,
    uint8_t                      initial_value );

// Set pin to value.
void
digital_io_pin_set (digital_io_pin_name_t pin, uint8_t value);

// Initialize pin of port for input or output (as per direction argument).
// If the pin is configured for input, then enable_pullup determines whether
// the internal pullup resistor is enabled.  If configured for ouput,
// initial_value determines the initial value of the pin.
void
dio_pin_initialize (char port, uint8_t pin, digital_io_pin_direction_t direction, 
  uint8_t enable_pullup, int8_t initial_value);

// Set output pin of port to value.
void
dio_pin_set (char port, uint8_t pin, uint8_t value);

#endif // DIGITAL_IO_PIN_H
