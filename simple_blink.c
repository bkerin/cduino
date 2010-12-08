
#include <avr/io.h>
#include <util/delay.h>

#include "util.h"

static void
set_pin_pd7_for_output (uint8_t initial_value)
{
  DDRD |= _BV (DDD7);  
  loop_until_bit_is_set (DDRD, DDD7);

  if ( initial_value == HIGH ) {
    PORTD |= _BV (PORTD7);
    loop_until_bit_is_set (PORTD, PORTD7);
  }
  else {
    PORTD &= ~(_BV (PORTD7));
    loop_until_bit_is_clear (PORTD, PORTD7);
  }
}

static void
set_pin_pd7 (uint8_t value)
{
  if ( value == HIGH ) {
    PORTD |= _BV (PORTD7);
    loop_until_bit_is_set (PORTD, PORTD7);
  }
  else {
    PORTD &= ~(_BV (PORTD7));
    loop_until_bit_is_clear (PORTD, PORTD7);
  }
}

static void
set_pin_pb5_for_output (uint8_t initial_value)
{
  DDRB |= _BV (DDB5);  
  loop_until_bit_is_set (DDRB, DDB5);

  if ( initial_value == HIGH ) {
    PORTB |= _BV (PORTB5);
    loop_until_bit_is_set (PORTB, PORTB5);
  }
  else {
    PORTB &= ~(_BV (PORTB5));
    loop_until_bit_is_clear (PORTB, PORTB5);
  }
}

static void
set_pin_pb5 (uint8_t value)
{
  if ( value == HIGH ) {
    PORTB |= _BV (PORTB5);
    loop_until_bit_is_set (PORTB, PORTB5);
  }
  else {
    PORTB &= ~(_BV (PORTB5));
    loop_until_bit_is_clear (PORTB, PORTB5);
  }
}


int
main (void)
{
  set_pin_pd7_for_output (HIGH);

  const double blink_time_ms = 400;

  while ( 1 ) {
    _delay_ms (blink_time_ms);
    set_pin_pd7 (LOW);
    set_pin_pb5 (LOW);
    _delay_ms (blink_time_ms);
    set_pin_pd7 (HIGH);
    set_pin_pb5 (HIGH);
  }
}

