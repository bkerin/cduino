// Implementation of the interface described in dio_pin.h.

#include <assert.h>
#include <stdlib.h>
#include <avr/io.h>

#include "dio_pin.h"

// Microcontrollers seem to use "PINx" to mean "register which receives
// input" and "PORTx" to mean "register to which output (for output pins)
// or pullup resistor status (for input pins) is written.

#define PINB_REGISTER 0x03
#define DDRB_REGISTER 0x04
#define PORTB_REGISTER 0x05

#define PINC_REGISTER 0x06
#define DDRC_REGISTER 0x07
#define PORTC_REGISTER 0x08

#define PIND_REGISTER 0x09
#define DDRD_REGISTER 0x0A
#define PORTD_REGISTER 0x0B

static void
lookup_pin_registers (
    digital_io_pin_name_t   pin,
    uint8_t                 *pin_register,
    uint8_t                 *dd_register,
    uint8_t                 *port_register,
    uint8_t                  *port_pin_number )
{
  // This code saves a bit of code and a few comparisons by depending on
  // the order of the constants in the digital_io_pin_name_t enumeration
  // (including the values that don't guarantee a particular value to the
  // client).  So we make a few assertions out of paranoia.
  assert (DIGITAL_IO_PIN_PB6 == 14); 
  assert (DIGITAL_IO_PIN_PC5 == 21); 
  if ( pin <= 7 ) {
    *pin_register = PIND_REGISTER;
    *dd_register = DDRD_REGISTER;
    *port_register = PORTD_REGISTER;
    *port_pin_number = pin;
  }
  else if ( pin > 15 ) {
    *pin_register = PINC_REGISTER;
    *dd_register = DDRC_REGISTER;
    *port_register = PORTC_REGISTER;
    *port_pin_number = pin - 16;
  }
  else {
    *pin_register = PINB_REGISTER;
    *dd_register = DDRB_REGISTER;
    *port_register = PORTB_REGISTER;
    *port_pin_number = pin - 8;
  }
}

void
digital_io_pin_init (
    digital_io_pin_name_t        pin,
    digital_io_pin_direction_t   direction,
    uint8_t                      enable_pullup,
    uint8_t                      initial_value )
{
  uint8_t pin_reg, dd_reg, port_reg;
  uint8_t ppn;   // Port pin number (0-7).
  lookup_pin_registers (pin, &pin_reg, &dd_reg, &port_reg, &ppn);

  if ( direction == DIGITAL_IO_PIN_DIRECTION_INPUT ) {
    _SFR_IO8 (dd_reg) &= ~(_BV (ppn));
    loop_until_bit_is_clear (_SFR_IO8 (dd_reg), ppn);
    if ( enable_pullup ) {
      _SFR_IO8 (port_reg) |= _BV (ppn);
      loop_until_bit_is_set (_SFR_IO8 (port_reg), ppn);
    }
    else {
      _SFR_IO8 (port_reg) &= ~(_BV (ppn));
      loop_until_bit_is_clear (_SFR_IO8 (port_reg), ppn);
    }
  }
  else {
    _SFR_IO8 (dd_reg) |= _BV (pin);
    loop_until_bit_is_set (_SFR_IO8 (dd_reg), pin);
    if ( initial_value ) {
      _SFR_IO8 (port_reg) |= _BV (pin);
      loop_until_bit_is_set (_SFR_IO8 (port_reg), pin);
    }
    else {
      _SFR_IO8 (port_reg) &= ~(_BV (pin));
      loop_until_bit_is_clear (_SFR_IO8 (port_reg), pin);
    }
  }
}

void
digital_io_pin_set (digital_io_pin_name_t pin, uint8_t value)
{
  uint8_t pin_reg, dd_reg, port_reg;   // Register values.
  uint8_t ppn;   // Port pin number (0-7).
  lookup_pin_registers (pin, &pin_reg, &dd_reg, &port_reg, &ppn);
    
  if ( value ) {
    _SFR_IO8 (port_reg) |= _BV (pin);
    loop_until_bit_is_set (_SFR_IO8 (port_reg), pin);
  }
  else {
    _SFR_IO8 (port_reg) &= ~(_BV (pin));
    loop_until_bit_is_clear (_SFR_IO8 (port_reg), pin);
  }
}

void
dio_pin_initialize (char port, uint8_t pin, digital_io_pin_direction_t direction, 
  uint8_t enable_pullup, int8_t initial_value)
{
  switch ( port ) {
    case 'B':
    case 'b':
      if ( direction == DIGITAL_IO_PIN_DIRECTION_INPUT ) {
        DDRB &= ~(_BV (pin));
        loop_until_bit_is_clear (DDRB, pin);
        if ( enable_pullup ) {
          PORTB |= _BV (pin);
          loop_until_bit_is_set (PORTB, pin);
        }
        else {
          PORTB &= ~(_BV (pin));
          loop_until_bit_is_clear (PORTB, pin);
        }
      }
      else {
        DDRB |= _BV (pin);
        loop_until_bit_is_set (DDRB, pin);
        if ( initial_value ) {
          PORTB |= _BV (pin);
          loop_until_bit_is_set (PORTB, pin);
        }
        else {
          PORTB &= ~(_BV (pin));
          loop_until_bit_is_clear (PORTB, pin);
        }
      }
      break;

    case 'C':
    case 'c':
      if ( direction == DIGITAL_IO_PIN_DIRECTION_INPUT ) {
        DDRC &= ~(_BV (pin));
        loop_until_bit_is_clear (DDRC, pin);
        if ( enable_pullup ) {
          PORTC |= _BV (pin);
          loop_until_bit_is_set (PORTC, pin);
        }
        else {
          PORTC &= ~(_BV (pin));
          loop_until_bit_is_clear (PORTC, pin);
        }
      }
      else {
        DDRC |= _BV (pin);
        loop_until_bit_is_set (DDRC, pin);
        if ( initial_value ) {
          PORTC |= _BV (pin);
          loop_until_bit_is_set (PORTC, pin);
        }
        else {
          PORTC &= ~(_BV (pin));
          loop_until_bit_is_clear (PORTC, pin);
        }
      }
      break;

    case 'D':
    case 'd':
      if ( direction == DIGITAL_IO_PIN_DIRECTION_INPUT ) {
        DDRD &= ~(_BV (pin));
        loop_until_bit_is_clear (DDRD, pin);
        if ( enable_pullup ) {
          PORTD |= _BV (pin);
          loop_until_bit_is_set (PORTD, pin);
        }
        else {
          PORTD &= ~(_BV (pin));
          loop_until_bit_is_clear (PORTD, pin);
        }
      }
      else {
        DDRD |= _BV (pin);
        loop_until_bit_is_set (DDRD, pin);
        if ( initial_value ) {
          PORTD |= _BV (pin);
          loop_until_bit_is_set (PORTD, pin);
        }
        else {
          PORTD &= ~(_BV (pin));
          loop_until_bit_is_clear (PORTD, pin);
        }
      }
      break;

    default:
      // Shouldn't be here.
      break;
  }
}

void
dio_pin_set (char port, uint8_t pin, uint8_t value)
{
  switch ( port ) {
    case 'B':
    case 'b':
      if ( value ) {
        PORTB |= _BV (pin);
        loop_until_bit_is_set (PORTB, pin);
      }
      else {
        PORTB &= ~(_BV (pin));
      }
      break;
    case 'C':
    case 'c':
      if ( value ) {
        PORTC |= _BV (pin);
        loop_until_bit_is_set (PORTC, pin);
      }
      else {
        PORTC &= ~(_BV (pin));
      }
      break;
    case 'D':
    case 'd':
      if ( value ) {
        PORTD |= _BV (pin);
        loop_until_bit_is_set (PORTD, pin);
      }
      else {
        PORTD &= ~(_BV (pin));
      }
      break;
    default:
      // Shouldn't be here
      break;
  }
}

