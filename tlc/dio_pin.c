// Implementation of the interface described in dio_pin.h.

#include <avr/io.h>

#include "dio_pin.h"

void
dio_pin_initialize (char port, uint8_t pin, dio_pin_direction_t direction, 
  uint8_t enable_pullup, int8_t initial_value)
{
  switch ( port ) {
    case 'B':
    case 'b':
      if ( direction == DIO_PIN_DIRECTION_INPUT ) {
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
      if ( direction == DIO_PIN_DIRECTION_INPUT ) {
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
      if ( direction == DIO_PIN_DIRECTION_INPUT ) {
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

